#include "common.h"
#include <algorithm>
#include <new>
#include <utility>
#include "appstate.h"
#include "text_io.h"
#include "colony_io.h"
#include "turn_io.h"
#include "production_io.h"
#include "heap_io.h"
#include "part_query_io.h"
#include "race_query_io.h"
#include "alert_io.h"
#include "random_io.h"
#include "record_io.h"

std::int16_t rgpctMineHit[3] = {3, 10, 35};
std::int16_t rgiWarpSafe[3] = {4, 6, 5};
std::int16_t rgrgdmgMinMine[3][2] = {{500, 600}, {2000, 2500}};
std::int16_t rgrgdmgMine[3][2] = {{100, 125}, {500, 600}};

// InitGameStuff sets up what the game code needs before it loads a game:
// the log and message buffers, the report id lists and the default
// production template, which ReadIniSettings also sets when stars.ini has
// none. FCreateStuff calls it for the Windows game.
void InitGameStuff(APPSESSION &app) {
    app.state.gd.fNoIdleChecks = 0;
    app.state.gd.fAisDone = 0;
    app.state.vplr = vrgplrDef[0];
    app.state.lpLog = LpAlloc<std::uint8_t>(app, 32000, htLog);
    app.state.lpMsg = LpAlloc<std::uint8_t>(app, 0xffc8, htMsg);
    app.state.vlprgidPlanet = LpAlloc<std::uint16_t>(app, 0x800, htPerm);
    app.state.vlprgidFleet = LpAlloc<std::uint16_t>(app, 0x800, htPerm);
    TextIO().pfnCchGetString(app, idsDefault, std::span{app.state.vrgZipProd[0].szName});
    app.state.vrgZipProd[0].fValid = 1;
    return;
}

std::int16_t FGenerateTurn(APPSESSION &app) {
    std::int16_t  fErrSav;
    char    *pchT;
    std::int16_t  ish;
    std::int16_t  j;
    std::uint8_t  mpiplr2[16];
    std::uint8_t  rgfNoXFile[16];
    std::int16_t  ifl;
    FLEET   *lpfl;
    char    *pchCur;
    std::int16_t  i;
    char     szT[256];
    std::int16_t  idCur;
    std::int16_t  fFollow;
    char    *pchBak;
    std::int16_t  fSuccess;
    std::int16_t  fDone;
    FLEET   *lpflTarget;
    ORDER    ord;
    std::int16_t  cAdv;
    PLANET  *lppl;
    PLANET  *lpplMac;
    std::int16_t  dPlanRange;
    std::int16_t  dRange;
    std::int16_t  iSteal;
    std::int16_t  pctDetect;

    idCur = app.state.idPlayer;
    fSuccess = 0;
    DestroyCurGame(app);
    if (app.state.gd.fTutorial) {
        Randomize(app, 1234567890);
    }
    fErrSav = app.state.fFileErrSilent;
    app.state.fFileErrSilent = 1;
    UpdateProgressGauge(app, static_cast<ProgressStep>(360));
    if (!FLoadGame(app, app.state.szBase, "hst")) {
        app.state.fFileErrSilent = fErrSav;
        TurnLog(app, idsCantFindHostFile);
        return 0;
    }
    TurnLog(app, idsGeneratingYearD);
    app.state.fFileErrSilent = fErrSav;
    if (VerMajor(app.state.wVersFile) <= 0) {
        for (i = 0; i < app.state.game.cPlayer && app.state.rgplr[i].iPlrBmp == 0; i++) {
        }
        if (i == app.state.game.cPlayer) {
            for (i = 0; i < app.state.game.cPlayer; i++) {
                app.state.rgplr[i].iPlrBmp = i;
            }
        }
    }
    StarsRecoveryScope scopeRecovery;
    try {
        app.state.lpcd = LpAlloc<COLDROP>(app, 1000 * sizeof(COLDROP), htMisc);
        app.state.lpxf = LpAlloc<XFERFULL>(app, 1000 * sizeof(XFERFULL), htMisc);
        app.state.vrgPlanResExtra = LpAlloc<std::uint16_t>(app, app.state.game.cPlanMax * 2, htMisc);
        std::fill_n(app.state.vrgPlanResExtra, app.state.game.cPlanMax, 0);
        app.state.vrgwtPopScore = LpAlloc<std::int32_t>(app, app.state.game.cPlanMax * sizeof(std::int32_t), htMisc);
        app.state.vrgiplrPopScore = LpAlloc<std::int16_t>(app, app.state.game.cPlanMax * sizeof(std::int16_t), htMisc);
        std::fill_n(app.state.vrgiplrPopScore, app.state.game.cPlanMax, std::int16_t{-1});
        UpdateProgressGauge(app, static_cast<ProgressStep>(370));
        app.state.cColDrop = 0;
        app.state.cXferFull = 0;
        app.state.gd.fGeneratingTurn = 1;
        app.state.gd.fRetryOpens = 1;
        app.state.imemMsgCur = 0;
        for (i = 0; i < app.state.game.cPlayer; i++) {
            mpiplr2[i] = i;
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            j = RandomIO().pfnRandom(app, app.state.game.cPlayer - i) + i;
            if (j != i) {
                idCur = mpiplr2[j];
                mpiplr2[j] = mpiplr2[i];
                mpiplr2[i] = idCur;
            }
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            j = mpiplr2[i];
            CchSprintf(app.state.szWork, "%s.x%d", app.state.szBase, j + 1);
            app.state.idPlayer = j;
            if (FLoadLogFile(app, app.state.szWork) && !FRunLogFile(app)) {
                AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsPlayerLogFileAppearsCorruptUnableLoad, nullptr), win32::MB_ICONHAND);
                goto FreeStuffUp;
            }
            UpdateProgressGauge(app, static_cast<ProgressStep>(LMulDiv(60, i + 1, app.state.game.cPlayer) + 370));
        }
        app.state.idPlayer = iplrNone;
        for (i = 0; i < app.state.game.cPlayer; i++) {
            for (ish = 0; ish < 16; ish++) {
                if (!app.state.rglpshdef[i][ish].fFree && app.state.rglpshdef[i][ish].hul.rghs[0].grhst != hstEngine) {
                    app.state.rglpshdef[i][ish].hul.rghs[0].grhst = hstEngine;
                    app.state.rglpshdef[i][ish].hul.rghs[0].iItem = iengineQuickJump5;
                    if (app.state.rglpshdef[i][ish].hul.rghs[0].cItem < 1) {
                        app.state.rglpshdef[i][ish].hul.rghs[0].cItem = 1;
                    }
                }
            }
        }
        fFollow = 0;
        for (ifl = 0; ifl < app.state.cFleet; ifl++) {
            lpfl = app.state.rglpfl[ifl];
            if (!app.state.rglpfl[ifl])
                break;
            lpfl->fNoHeal = 0;
            if (lpfl->cord == 1 && lpfl->lpplord->rgord()[0].grobj == grobjFleet) {
                fFollow = 1;
                lpfl->fMark = 1;
            } else {
                if (lpfl->lpplord->rgord()[0].grobj == grobjFleet && lpfl->cord == 1) {
                    TurnIO().pfnMessage(app, lpfl->iPlayer, idmHadOrdersFollowFleetWhichDidntMove, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0, 0, 0, 0, 0, 0);
                }
                lpfl->fMark = 0;
            }
        }
        ValidateWaypoints(app);
        if (fFollow) {
            fFollow = 1;
            for (i = 0; i < 8 && fFollow != 0; i++) {
                fFollow = 0;
                for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                    lpfl = app.state.rglpfl[ifl];
                    if (!app.state.rglpfl[ifl])
                        break;
                    if (lpfl->fMark && lpfl->cord == 1) {
                        ord = lpfl->lpplord->rgord()[0];
                        if (ord.grobj != grobjFleet) {
                        LUnmark:
                            lpfl->fMark = 0;
                            continue;
                        }
                        lpflTarget = LpflFromId(app, ord.id);
                        if (!lpflTarget || (lpflTarget->cord == 1 && lpflTarget->lpplord->rgord()[0].grobj != grobjFleet)) {
                            TurnIO().pfnMessage(app, lpfl->iPlayer, idmHadOrdersFollowFleetWhichDidntMove, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0, 0, 0, 0, 0, 0);
                            goto LUnmark;
                        }
                        if (lpflTarget->cord == 1)
                            continue;
                        fFollow = 1;
                        if (lpfl->lpplord->iordMax <= 1) {
                            lpfl->lpplord = LpplReAlloc(app, lpfl->lpplord, 2);
                        }
                        lpfl->lpplord->rgord()[1] = lpflTarget->lpplord->rgord()[1];
                        lpfl->lpplord->rgord()[1].txp = lpfl->lpplord->rgord()[0].txp;
                        lpfl->cord = 2;
                        lpfl->lpplord->iordMac = 2;
                    }
                }
            }
        }
        UpdateProgressGauge(app, static_cast<ProgressStep>(440));
        DoOrders(app, 0);
        UpdateProgressGauge(app, static_cast<ProgressStep>(530));
        for (i = 0; i < app.state.game.cPlayer; i++) {
            for (j = 0; j < 16; j++) {
                SetRaceStat(&app.state.rgplr[i], static_cast<RaceStat>(j), RaceQueryIO().pfnStat(&app.state.rgplr[i], static_cast<RaceStat>(j)));
            }
            if (app.state.rgplr[i].pctResearch < 0 || app.state.rgplr[i].pctResearch > 100) {
                app.state.rgplr[i].pctResearch = 15;
            }
            if (app.state.rgplr[i].pctIdealGrowth < 0) {
                app.state.rgplr[i].pctIdealGrowth = 1;
            }
            if (app.state.rgplr[i].pctIdealGrowth > 20) {
                app.state.rgplr[i].pctIdealGrowth = 20;
            }
            j = app.state.rgplr[i].fHacker;
            cAdv = CAdvantagePoints(app, &app.state.rgplr[i]);
            if ((cAdv < 0 || j != app.state.rgplr[i].fHacker) && !app.state.rgplr[i].fAi) {
                TurnIO().pfnMessage2(app, i, idmRaceDefinitionHasTamperedStatisticsHaveAltered, gotoNone, 0, 0);
                for (j = 0; j < app.state.game.cPlayer; j++) {
                    if (i != j && !app.state.rgplr[i].fAi) {
                        TurnIO().pfnMessage2(app, j, idmHackedRaceDiscoveredRaceStatisticsHaveAltered, gotoNone, i, 0);
                    }
                }
                app.state.rgplr[i].fHacker = 1;
                if (cAdv < 500) {
                    while (app.state.rgplr[i].rgAttr[0] < 25) {
                        app.state.rgplr[i].rgAttr[0]++;
                        cAdv = CAdvantagePoints(app, &app.state.rgplr[i]);
                        if (cAdv >= 500)
                            break;
                    }
                }
                if (cAdv < 500) {
                    while (app.state.rgplr[i].pctIdealGrowth > 1) {
                        app.state.rgplr[i].pctIdealGrowth--;
                        cAdv = CAdvantagePoints(app, &app.state.rgplr[i]);
                        if (cAdv >= 500)
                            break;
                    }
                }
                if (cAdv < 500) {
                    for (j = 8; j <= 13; j++) {
                        app.state.rgplr[i].rgAttr[j] = 0;
                        cAdv = CAdvantagePoints(app, &app.state.rgplr[i]);
                        if (cAdv >= 500)
                            break;
                    }
                }
            }
        }
        UnmarkMineFields(app);
        MoveThings(app, 0);
        UpdateProgressGauge(app, static_cast<ProgressStep>(550));
        MoveFleets(app);
        lppl = app.state.lpPlanets;
        lpplMac = app.state.lpPlanets + app.state.cPlanet;
        for (; lppl < lpplMac; lppl++) {
            lppl->fHomeworld = 0;
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            app.state.lpPlanets[app.state.rgplr[i].idPlanetHome].fHomeworld = 1;
        }
        UpdateProgressGauge(app, static_cast<ProgressStep>(650));
        ThingDecay(app);
        BreedColonistsInTransit(app);
        UpdateProgressGauge(app, static_cast<ProgressStep>(700));
        Produce(app);
        UpdateProgressGauge(app, static_cast<ProgressStep>(750));
        MoveThings(app, 1);
        UpdateProgressGauge(app, static_cast<ProgressStep>(770));
        FuelFleets(app);
        DoOrders(app, 1);
        SweepForMines(app);
        HealShips(app);
        AutoTerraform(app);
        RemoteTerraforming(app);
        UpdateProgressGauge(app, static_cast<ProgressStep>(850));
        ValidateWaypoints(app);
        UpdateGuesses(app);
        UpdateProgressGauge(app, static_cast<ProgressStep>(852));
        FMarkFile(app, dtHost, iplrNone, mdMarkInUse, 0);
        CreateBackupDir(app);
        app.state.game.turn++;
        pchCur = &app.state.szBase[std::strlen(app.state.szBase)];
        pchT = std::strrchr(app.state.szBase, chDirSep);
        std::strcpy(szT, app.state.szBackup);
        if (!pchT) {
            std::strcat(szT, app.state.szBase);
        } else {
            std::strcat(szT, pchT + 1);
        }
        pchBak = &szT[std::strlen(szT)];
        UpdateProgressGauge(app, static_cast<ProgressStep>(854));
        UpdatePlayerScores(app);
        for (i = 0; i < app.state.game.cPlayer; i++) {
            for (j = 0; j < 10; j++) {
                if (!app.state.rglpshdefSB[i][j].fFree) {
                    app.state.rglpshdefSB[i][j].SetLVisible(static_cast<std::int16_t>((100 - PctCloakFromHuldef(app, &app.state.rglpshdefSB[i][j].hul, i, nullptr))));
                    app.state.rglpshdefSB[i][j].SetLVisible(static_cast<std::uint32_t>((app.state.rglpshdefSB[i][j].lVisible() * app.state.rglpshdefSB[i][j].lVisible())));
                }
            }
            for (j = 0; j < 16; j++) {
                if (!app.state.rglpshdef[i][j].fFree) {
                    dRange = GetShdefScannerRange(app, app.state.rglpshdef[i] + j, i, &dPlanRange, &pctDetect, &iSteal);
                    app.state.rglpshdef[i][j].dScanRange = dRange;
                    app.state.rglpshdef[i][j].dScanRange2 = dPlanRange;
                    app.state.rglpshdef[i][j].pctDetect = pctDetect;
                    app.state.rglpshdef[i][j].iSteal = iSteal;
                    if (!ProductionIO().pfnBuildable(app, app.state.rglpshdef[i] + j, i)) {
                        app.state.rglpshdef[i][j].fGift = 1;
                    }
                }
            }
        }
        j = 856;
        fDone = 0;
        std::ranges::fill(rgfNoXFile, 0);
        i = 0;
        while (!fDone) {
            UpdateProgressGauge(app, static_cast<ProgressStep>(j));
            j += 17 / (app.state.game.cPlayer + 1);
            if (i >= app.state.game.cPlayer) {
                i = -1;
                fDone = 1;
            }
            if (i >= 0) {
                CchSprintf(std::span{app.state.szBase}.subspan(static_cast<std::size_t>(pchCur - app.state.szBase)), ".x%d", i + 1);
                std::strcpy(pchBak, pchCur);
                RemoveNativeFile(szT);
                if (!FFileExists(app.state.szBase)) {
                    rgfNoXFile[i] = 1;
                } else {
                    RenameNativeFile(app.state.szBase, szT);
                }
                pchBak[1] = 'm';
                pchCur[1] = 'm';
            } else {
                std::strcpy(pchCur, ".hst");
                std::strcpy(pchBak, ".hst");
            }
            RemoveNativeFile(szT);
            if (i >= 0 && rgfNoXFile[i]) {
                StarsCopyFile(app, app.state.szBase, szT);
            } else {
                RenameNativeFile(app.state.szBase, szT);
            }
            *pchCur = 0;
            i++;
        }
        j = 875;
        fDone = 0;
        app.state.game.wGen = static_cast<std::uint16_t>(RandomIO().pfnRandom(app, 8));
        i = 0;
        while (!fDone) {
            UpdateProgressGauge(app, static_cast<ProgressStep>(j));
            j += 122 / (app.state.game.cPlayer + 1);
            if (i >= app.state.game.cPlayer) {
                i = -1;
                fDone = 1;
            }
            FWriteDataFile(app, app.state.szBase, i, i != -1 && rgfNoXFile[i]);
            i++;
        }
        UpdateProgressGauge(app, static_cast<ProgressStep>(998));
        app.state.imemLogCur = 0;
        fSuccess = 1;
    } catch (const StarsError &) {
    }
FreeStuffUp:
    UpdateProgressGauge(app, static_cast<ProgressStep>(1000));
    HeapIO().pfnFree(app, app.state.vrgPlanResExtra, htMisc);
    app.state.vrgPlanResExtra = nullptr;
    HeapIO().pfnFree(app, app.state.vrgwtPopScore, htMisc);
    app.state.vrgwtPopScore = nullptr;
    HeapIO().pfnFree(app, app.state.vrgiplrPopScore, htMisc);
    app.state.vrgiplrPopScore = nullptr;
    HeapIO().pfnFree(app, app.state.lpcd, htMisc);
    app.state.lpcd = nullptr;
    HeapIO().pfnFree(app, app.state.lpxf, htMisc);
    app.state.lpxf = nullptr;
    app.state.gd.fGeneratingTurn = 0;
    app.state.gd.fRetryOpens = 0;
    app.state.idPlayer = iplrNone;
    if (fSuccess && app.state.ini.fGen) {
        app.state.vretExitValue = 1;
    }
    TurnLog(app, static_cast<StringId>(idsFailed + fSuccess));
    return fSuccess;
}

void EnsureAis(APPSESSION &app) {
    std::int16_t fHostSav;
    std::int16_t fErrSav;
    std::int16_t fOpened;
    std::int16_t fWorkDone;
    std::int16_t fSubmitSav;
    std::int16_t iPlayer;
    MDPLR   rgmdplr[16];

    fSubmitSav = app.state.gd.fSubmit;
    fWorkDone = 0;
    if (!app.state.gd.fAisDone) {
        fHostSav = app.state.gd.fHostMode;
        if (!app.state.gd.fHostMode) {
            DestroyCurGame(app);
            FLoadGame(app, app.state.szBase, "hst");
        }
        for (iPlayer = 0; iPlayer < app.state.game.cPlayer; iPlayer++) {
            rgmdplr[iPlayer].SetWMdPlr(app.state.rgplr[iPlayer].wMdPlr());
        }
        app.state.gd.fSubmit = 1;
        fErrSav = app.state.fFileErrSilent;
        app.state.fFileErrSilent = 1;
        for (iPlayer = 0; iPlayer < app.state.game.cPlayer; iPlayer++) {
            UpdateProgressGauge(app, static_cast<ProgressStep>(LMulDiv(340, iPlayer + 1, app.state.game.cPlayer)));
            if (rgmdplr[iPlayer].fAi) {
                fWorkDone = 1;
                app.state.gd.fGeneratingTurn = 1;
                app.state.gd.fHostMode = 1;
                fOpened = FOpenFile(app, dtLog, iPlayer, 32);
                app.state.gd.fGeneratingTurn = 0;
                app.state.gd.fHostMode = fHostSav;
                if (fOpened) {
                    RecordIO().pfnStreamClose(app);
                } else {
                    DoAiTurn(app, iPlayer, rgmdplr[iPlayer].wMdPlr());
                }
            }
        }
        app.state.gd.fSubmit = fSubmitSav;
        if (fWorkDone) {
            DestroyCurGame(app);
            FLoadGame(app, app.state.szBase, "hst");
        }
        app.state.fFileErrSilent = fErrSav;
        app.state.gd.fAisDone = 1;
    }
    return;
}

void VerifyTurns(APPSESSION &app) {
    std::int16_t idsError;
    std::int16_t idCur;
    std::int16_t cAi;
    std::int16_t i;
    std::int16_t cOut;
    std::int16_t fOut;

    idCur = app.state.idPlayer;
    cOut = 0;
    cAi = 0;
    app.state.lpcd = LpAlloc<COLDROP>(app, 1000 * sizeof(COLDROP), htMisc);
    app.state.lpxf = LpAlloc<XFERFULL>(app, 1000 * sizeof(XFERFULL), htMisc);
    app.state.vrgPlanResExtra = LpAlloc<std::uint16_t>(app, app.state.game.cPlanMax * 2, htMisc);
    std::fill_n(app.state.vrgPlanResExtra, app.state.game.cPlanMax, 0);
    app.state.cColDrop = 0;
    app.state.cXferFull = 0;
    app.state.imemMsgCur = 0;
    for (i = 0; i < app.state.game.cPlayer; i++) {
        fOut = app.state.rgOut[i];
        idsError = 0;
        if (app.state.rgplr[i].fAi || FCheckLogFile(app, i, &idsError)) {
            if (app.state.rgplr[i].fAi) {
                cAi++;
                app.state.rgOut[i] = 0;
            } else {
                CchSprintf(app.state.szWork, "%s.x%d", app.state.szBase, i + 1);
                app.state.idPlayer = i;
                if (FLoadLogFile(app, app.state.szWork) && !FRunLogFile(app)) {
                    app.state.rgOut[i] = 3;
                } else {
                    app.state.rgOut[i] = 0;
                }
            }
        } else if (idsError != 0) {
            switch (idsError) {
            case idsFileGame:
                app.state.rgOut[i] = 5;
                break;
            case idsFileDate:
                app.state.rgOut[i] = 4;
                break;
            default:
                app.state.rgOut[i] = 3;
                break;
            }
            cOut++;
        } else if (app.state.rgplr[i].fDead) {
            app.state.rgOut[i] = -1;
        } else if (app.state.gd.fPartialTurn) {
            app.state.rgOut[i] = 2;
            cOut++;
        } else {
            app.state.rgOut[i] = 1;
            cOut++;
        }
        if (app.state.ctickLast == 0 || app.state.rgOut[i] != fOut) {
            app.state.ctickLast = DwTickCount();
        }
    }
    HeapIO().pfnFree(app, app.state.vrgPlanResExtra, htMisc);
    app.state.vrgPlanResExtra = nullptr;
    HeapIO().pfnFree(app, app.state.lpcd, htMisc);
    app.state.lpcd = nullptr;
    HeapIO().pfnFree(app, app.state.lpxf, htMisc);
    app.state.lpxf = nullptr;
    app.state.idPlayer = idCur;
    return;
}

std::int16_t CTurnsOutSafe(APPSESSION &app) {
    std::int16_t idPlayerSav;
    std::int16_t fHostModeSav;
    std::int16_t fGenSav;
    std::int16_t cturn;

    fHostModeSav = app.state.gd.fHostMode;
    fGenSav = app.state.gd.fGeneratingTurn;
    idPlayerSav = app.state.idPlayer;
    app.state.idPlayer = iplrNone;
    app.state.gd.fHostMode = 1;
    app.state.gd.fGeneratingTurn = 0;
    cturn = CFindTurnsOutstanding(app);
    app.state.gd.fGeneratingTurn = fGenSav;
    app.state.gd.fHostMode = fHostModeSav;
    app.state.idPlayer = idPlayerSav;
    return cturn;
}

std::int16_t CFindTurnsOutstanding(APPSESSION &app) {
    std::int16_t idsError;
    std::int16_t cAi;
    std::int16_t i;
    std::int16_t cOut;
    std::int16_t fSav;
    std::int16_t fOut;

    cOut = 0;
    cAi = 0;
    fSav = app.state.fFileErrSilent;
    app.state.fFileErrSilent = 1;
    app.state.gd.fGeneratingTurn = 1;
    for (i = 0; i < app.state.game.cPlayer; i++) {
        fOut = app.state.rgOut[i];
        idsError = 0;
        if (app.state.rgplr[i].fAi || FCheckLogFile(app, i, &idsError)) {
            if (app.state.rgplr[i].fAi) {
                cAi++;
            }
            app.state.rgOut[i] = 0;
        } else if (idsError != 0) {
            switch (idsError) {
            case idsFileGame:
                app.state.rgOut[i] = 5;
                break;
            case idsFileDate:
                app.state.rgOut[i] = 4;
                break;
            default:
                app.state.rgOut[i] = 3;
                break;
            }
            cOut++;
        } else if (app.state.rgplr[i].fDead) {
            app.state.rgOut[i] = -1;
        } else if (app.state.gd.fPartialTurn) {
            app.state.rgOut[i] = 2;
            cOut++;
        } else {
            app.state.rgOut[i] = 1;
            cOut++;
        }
        if (app.state.ctickLast == 0 || app.state.rgOut[i] != fOut) {
            app.state.ctickLast = DwTickCount();
        }
    }
    app.state.gd.fGeneratingTurn = 0;
    app.state.gd.fAllAis = cAi == app.state.game.cPlayer;
    app.state.fFileErrSilent = 0;
    return cOut;
}

static std::int16_t FCopyCmdPath(APPSESSION &app, std::string_view psz) {
    if (!FUtf8Valid(psz)) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsNativeFilenameEncoding).data(), win32::MB_ICONHAND);
        app.state.szBase[0] = 0; app.state.ini.fCmdLine = 0; app.state.ini.fStartupFile = 0;
        return 0;
    }
    if (psz.size() >= sizeof(app.state.szBase)) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsNativeFilenameTooLong).data(), win32::MB_ICONHAND);
        app.state.szBase[0] = 0; app.state.ini.fCmdLine = 0; app.state.ini.fStartupFile = 0;
        return 0;
    }
    CchCopyUtf8(std::span{app.state.szBase}, psz);
    return 1;
}

static bool FNextBatchPath(APPSESSION &app) {
    std::size_t ich;
    std::string_view psz;

    if (app.state.lpchBatch == app.state.lpchBatchMac) return false;
    ich = app.state.szBatch.find('\n', app.state.lpchBatch);
    if (ich == std::string::npos) ich = app.state.lpchBatchMac;
    psz = std::string_view(app.state.szBatch).substr(app.state.lpchBatch, ich - app.state.lpchBatch);
    app.state.lpchBatch = ich < app.state.lpchBatchMac ? ich + 1 : ich;
    if (psz.ends_with('\r')) psz.remove_suffix(1);
    if (psz.empty()) {
        app.state.szBase[0] = 0;
        FileError(app, idsCantOpenFile);
        return false;
    }
    return FCopyCmdPath(app, psz);
}

std::int16_t FSetUpBatchProcessing(APPSESSION &app) {
    std::int16_t fSuccess;
    std::optional<std::int64_t> cb;
    std::string szBatch;

    fSuccess = 0;
    app.state.szBatch.clear();
    app.state.lpchBatch = app.state.lpchBatchMac = 0;
    StarsRecoveryScope scopeRecovery;
    try {
        RecordIO().pfnStreamOpen(app, app.state.szBase, mdRead);
        cb = CbFileSize(app.state.hf);
        if (!cb) {
            FileError(app, idsGameFileAppearsCorruptUnableLoadFile);
            StarsThrowError(starsErrorFile, app.state.idsFileError);
        }
        try {
            if (std::cmp_greater(*cb, szBatch.max_size())) throw std::bad_alloc{};
            szBatch.resize(static_cast<std::size_t>(*cb));
        } catch (const std::bad_alloc &) {
            AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsMemory, nullptr), win32::MB_ICONHAND);
            StarsThrowError(starsErrorMemory, idsMemory);
        }
        RgFromStream(app, std::as_writable_bytes(std::span{szBatch}));
        app.state.szBatch = std::move(szBatch);
        app.state.lpchBatchMac = app.state.szBatch.size();
        fSuccess = FNextBatchPath(app);
    } catch (const StarsError &) {
    }
    RecordIO().pfnStreamClose(app);
    if (!fSuccess) {
        app.state.szBase[0] = 0;
        app.state.szBatch.clear();
        app.state.lpchBatch = app.state.lpchBatchMac = 0;
    }
    return fSuccess;
}

// ParseCmdLine reads the command line's switches into ini and gd and its
// file name into szBase; *pfSeed and *plSeed get a -s<seed>. The Windows
// game and stars-host take the same UTF-8 command line. Invalid encoding or
// capacity returns FALSE so neither caller executes a partially parsed command.
std::int16_t ParseCmdLine(APPSESSION &app, char *lpCmdLine, std::int16_t *pfSeed, std::uint32_t *plSeed) {
    char   *pch;
    char   *lpT;
    std::int16_t i;
    int cchPass;
    std::size_t cbPass;

    lpT = lpCmdLine;
    while (*lpT != 0) {
        for (; *lpT == ' '; lpT++) {
        }
        /* Windows switches may start with '/'; where '/' separates
           directories, only '-' does. */
        if (*lpT == '-' || (*lpT == '/' && chDirSep != '/')) {
            for (lpT++; *lpT != 0 && *lpT != ' '; lpT++) {
                switch (*lpT) {
                case 'W':
                case 'w':
                    app.state.ini.fWait = 1;
                    break;
                case 'D':
                case 'd':
                    for (lpT++; *lpT != 0 && *lpT != ' '; lpT++) {
                        switch (*lpT) {
                        case 'F':
                        case 'f':
                            app.state.ini.fDumpFleets = 1;
                            break;
                        case 'P':
                        case 'p':
                            app.state.ini.fDumpPlanets = 1;
                            break;
                        case 'M':
                        case 'm':
                            app.state.ini.fDumpMap = 1;
                        }
                    }
                    lpT--;
                    break;
                case 'G':
                case 'g':
                    app.state.ini.fGen = 1;
                    i = 0;
                    while (lpT[1] >= '0' && lpT[1] <= '9') {
                        lpT++;
                        i = 10 * i + *lpT - '0';
                        if (i > 1000) {
                            i = 1000;
                            for (; lpT[1] >= '0' && lpT[1] <= '9'; lpT++) {
                            }
                            break;
                        }
                    }
                    if (i <= 0)
                        break;
                    app.state.ini.cTurnGen = i - 1;
                    break;
                case 'A':
                case 'a':
                    app.state.ini.fNewGame = 1;
                    break;
                case 'H':
                case 'h':
                    app.state.gd.fHotSeat = 1;
                    break;
                case 'X':
                case 'x':
                    app.state.gd.fExitWindows = 1;
                    break;
                case 'B':
                case 'b':
                    for (lpT++; *lpT == ' '; lpT++) {
                    }
                    pch = lpT;
                    while (*lpT != 0 && *lpT != ' ') lpT++;
                    if (!FCopyCmdPath(app, std::string_view{pch, static_cast<std::size_t>(lpT - pch)})) return 0;
                    lpT--;
                    if (!FSetUpBatchProcessing(app))
                        return 0;
                    app.state.ini.fBatch = 1;
                    app.state.ini.fGen = 1;
                    app.state.ini.fStartupFile = 1;
                    app.state.ini.fCmdLine = 1;
                    break;
                case 'V':
                case 'v':
                    app.state.ini.fValidate = 1;
                    break;
                case 'L':
                case 'l':
                    app.state.ini.fLogging = 1;
                    break;
                case 'T':
                case 't':
                    app.state.ini.fTry = 1;
                    break;
                case 'C':
                case 'c':
                    app.state.ini.fCmdLine = app.state.szBase[0] != 0;
                    break;
                case 'S':
                case 's':
                    /* -s<seed>: a fixed startup seed instead of the clock, so
                       regression runs repeat exactly. Not in the original. */
                    *pfSeed = 1;
                    *plSeed = 0;
                    while (lpT[1] >= '0' && lpT[1] <= '9') {
                        lpT++;
                        *plSeed = 10 * *plSeed + static_cast<std::uint32_t>((*lpT - '0'));
                    }
                    break;
                case 'P':
                case 'p':
                    for (lpT++; *lpT == ' '; lpT++) {
                    }
                    pch = lpT;
                    while (*lpT != 0 && *lpT != ' ') lpT++;
                    cbPass = static_cast<std::size_t>((lpT - pch));
                    cchPass = CchCopyUtf8(app.state.szPassLast, sizeof(app.state.szPassLast), pch, cbPass);
                    if (cchPass < 0) {
                        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, cchPass == -3 ? idsNativeCommandPasswordTooLong : idsNativeCommandPasswordEncoding).data(), win32::MB_ICONHAND);
                        app.state.szBase[0] = 0; app.state.ini.fCmdLine = 0; app.state.ini.fStartupFile = 0;
                        return 0;
                    }
                    lpT--;
                    app.state.lSaltLast = LSaltFromSz(app.state.szPassLast);
                }
            }
        } else {
            pch = lpT;
            while (*lpT != 0 && *lpT != ' ') lpT++;
            if (!FCopyCmdPath(app, std::string_view{pch, static_cast<std::size_t>(lpT - pch)})) return 0;
            app.state.ini.fStartupFile = 1;
            app.state.ini.fCmdLine = 1;
        }
    }
    return 1;
}

// FRunCmdLine carries out a command line's host work: validating the host
// file (-v), creating a universe (-a), or generating turns (-g, with -b
// for a batch of games and -t to generate only when every turn is in). It
// returns FALSE when there is none to do now: the command line opens a
// game, or waits (-w) for turns still out, which the Windows game's host
// timer does.
std::int16_t FRunCmdLine(APPSESSION &app) {
    char    szTemp[80];
    std::int16_t ich;
    std::int16_t i;

    if (app.state.ini.fValidate) {
        app.state.fFileErrSilent = 1;
        ClearFile(app, 7);
        if (FLoadGame(app, app.state.szBase, "hst")) {
            VerifyTurns(app);
            DestroyCurGame(app);
            EnsureAis(app);
            CchSprintf(szTemp, "\"%s\" Year: %d", app.state.game.szName, app.state.game.turn + 2400);
            OutputSz(app, 7, szTemp);
            for (i = 0; i < app.state.game.cPlayer; i++) {
                if (app.state.rgOut[i] + 1 > 3) {
                    ich = CchSprintf(szTemp, "Error: %d: ", i + 1);
                } else {
                    ich = CchSprintf(szTemp, "%d: ", i + 1);
                }
                if (!app.state.gd.fNoHostNames) {
                    ich += CchSprintf(std::span{szTemp}.subspan(static_cast<std::size_t>(ich)), "\"%s\" ", TextIO().pfnPlayerName(app, i, 1, 1, 1, 0, nullptr));
                }
                CchAppendUtf8Prefix(std::span{szTemp}, TextIO().pfnCompressed(app, static_cast<StringId>(idsTurned + app.state.rgOut[i])).data());
                if (app.state.rgplr[i].fHacker) {
                    CchAppendUtf8Prefix(std::span{szTemp}, " - HACKER");
                }
                OutputSz(app, 7, szTemp);
            }
        }
        return 1;
    }
    if (app.state.ini.fNewGame) {
        GenNewGameFromFile(app, app.state.szBase);
        return 1;
    }
    if (!app.state.ini.fGen) {
        return 0;
    }
LBatchNext:
    if ((!app.state.ini.fWait && !app.state.ini.fTry) || CTurnsOutSafe(app) == 0) {
        EnsureAis(app);
        FGenerateTurn(app);
        if (app.state.ini.fBatch && app.state.lpchBatch < app.state.lpchBatchMac) {
        LTryNextBatch:
            DestroyCurGame(app);
            if (!FNextBatchPath(app)) {
                app.state.ini.fBatch = 0;
                app.state.ini.fGen = 0;
                return 1;
            }
            app.state.ini.fStartupFile = 1;
            goto LBatchNext;
        }
        if (app.state.ini.cTurnGen == 0)
            return 1;
        app.state.ini.cTurnGen--;
        goto LBatchNext;
    }
    if (app.state.ini.fTry) {
        if (app.state.ini.fBatch && app.state.lpchBatch < app.state.lpchBatchMac)
            goto LTryNextBatch;
        return 1;
    }
    return 0;
}

void DoOrders(APPSESSION &app, std::int16_t fPostMovement) {
    PLANET *lppl;
    PLANET *lpplMac;

    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        lppl->fWasInhabited = lppl->iPlayer != iplrNone;
    }
    if (fPostMovement) {
        app.state.idBattle = (app.state.game.turn & 0xf) * 0x100 + 1;
        DoBattles(app, fPostMovement);
    }
    DoThingInteractions(app, fPostMovement);
    if (fPostMovement) {
        lppl = app.state.lpPlanets;
        lpplMac = app.state.lpPlanets + app.state.cPlanet;
        for (; lppl < lpplMac; lppl++) {
            lppl->turn = 0;
        }
    }
    SatisfyOrders(app, !fPostMovement ? 1 : 3);
    DropColonists(app);
    TurnIO().pfnResearch(app, 0);
    SatisfyOrders(app, !fPostMovement ? 2 : 4);
    if (!fPostMovement) {
        TransferToOthers(app);
    }
    return;
}

void MoveThings(APPSESSION &app, std::int16_t fPostProd) {
    std::int16_t   k;
    std::int16_t   dUni;
    double    d;
    POINT16   pt;
    std::int16_t   iMax;
    POINT16   ptDst;
    std::int16_t   dLeft;
    THING    *lpth;
    std::int16_t   fAnythingMoved;
    std::int16_t   fMajorMove;
    MessageId idm;
    std::int16_t   iLow;
    POINT16   ptSrc;
    THING    *lpthMac;
    std::int16_t   dRange;
    POINT16   ptBase;
    std::int16_t   iX;
    std::int16_t   rgC[2];
    std::int16_t   rgwtTerra[3];
    std::int32_t   wtTot;
    std::int16_t   iWarp2;
    std::int16_t   iWarp;
    std::int16_t   fTerra;
    PLANET   *lppl;
    std::int16_t   wtCur;
    std::int16_t   pctMinKeep;
    std::int16_t   fTwoMAs;
    std::int32_t   lDefKilled;
    std::int32_t   lColKilled;
    std::int16_t   i;
    std::int16_t   pctCaught;
    float     pct;
    std::int32_t   dmgRaw;
    std::int16_t   iWarpPacket;
    std::int16_t   iWarpPacket2;
    THING    *lpth2;
    THING    *lpth2Mac;
    std::int16_t   pctRate;
    std::int16_t   iplr;
    std::int16_t   rgMin[3];
    std::int16_t   cTerraPerm;
    std::int16_t   cTerraTemp;
    std::int16_t   rgMax[3];
    std::int16_t   rgCost[3];
    double    dyRound;
    double    dxRound;
    double    r;

    // No retargeting message has been selected when this movement pass starts.
    idm = static_cast<MessageId>(idmNone);
    fAnythingMoved = 0;
    lpth = app.state.lpThings;
    lpthMac = app.state.lpThings + app.state.cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith() == ithWormhole && fPostProd) {
            k = 0;
            ptBase = lpth->pt();
            fMajorMove = RandomIO().pfnRandom(app, 100) < PctWormholeMoves(lpth);
            if (fMajorMove) {
                lpth->thw().SetGrbitPlr(0);
                dUni = 400 * app.state.game.mdSize + 400;
                lpth->thw().SetCLastMove(0);
            } else {
                lpth->thw().SetCLastMove(lpth->thw().cLastMove() + 1);
            }
            iMax = 16;
            while (k++ < 100) {
                if (fMajorMove) {
                    { const auto xForThing = (RandomIO().pfnRandom(app, dUni) + 1000); auto &thForThing = *(lpth); thForThing.pt().SetX(xForThing); }
                    { const auto yForThing = (RandomIO().pfnRandom(app, dUni) + 1000); auto &thForThing = *(lpth); thForThing.pt().SetY(yForThing); }
                } else {
                    { const auto xForThing = (RandomIO().pfnRandom(app, 25) + ptBase.x - 12); auto &thForThing = *(lpth); thForThing.pt().SetX(xForThing); }
                    { const auto yForThing = (RandomIO().pfnRandom(app, 25) + ptBase.y - 12); auto &thForThing = *(lpth); thForThing.pt().SetY(yForThing); }
                }
                if (lpth->pt().x() != ptBase.x || lpth->pt().y() != ptBase.y) {
                    iLow = IValidateWormholePos(app, lpth);
                    if (iLow == 0)
                        break;
                    if (iLow < iMax) {
                        iMax = iLow;
                        pt = lpth->pt();
                    }
                }
            }
            if (iLow != 0) {
                lpth->pt().Set(pt);
            }
        } else if (lpth->ith() == ithMysteryTrader && !fPostProd) {
            dRange = lpth->tht().iWarp();
            if (dRange < 13 && RandomIO().pfnRandom(app, 25) == 0) {
                idm = idmMysteryTraderHasUnexplicablyChangedHisCourse;
                if (RandomIO().pfnRandom(app, 3) != 0)
                    goto LSpeedUpOnly;
            LRetargetFreighter:
                if (RandomIO().pfnRandom(app, 2) == 0) {
                    rgC[0] = 400 * app.state.game.mdSize + 1380;
                } else {
                    rgC[0] = 1020;
                }
                rgC[1] = RandomIO().pfnRandom(app, 400 * app.state.game.mdSize + 361) + 1020;
                iX = RandomIO().pfnRandom(app, 2);
                lpth->tht().ptDest().SetX(rgC[iX]);
                lpth->tht().ptDest().SetY(rgC[iX == 0]);
            LSpeedUpOnly:
                dRange++;
                lpth->tht().SetIWarp(dRange);
                for (k = 0; k < app.state.game.cPlayer; k++) {
                    TurnIO().pfnMessage2(app, k, idm, gotoThing, lpth->idFull(), 0);
                }
            }
            dRange = lpth->tht().iWarp();
            dRange *= dRange;
            ptDst = lpth->tht().ptDest();
            fAnythingMoved = 1;
            if (idm == idmMysteryTraderHasDecidedMakeAnotherPass)
                continue;
            goto MoveTh;
        } else {
            if (lpth->ith() != ithMineralPacket || lpth->thp().iWarp() == 0 || (fPostProd && lpth->thp().fMoved()))
                continue;
            if (lpth->thp().rgwtMin(0) == 0 && lpth->thp().rgwtMin(1) == 0 && lpth->thp().rgwtMin(2) == 0)
                goto LFreeThePacket;
            lpth->thp().SetFMoved(1);
            fAnythingMoved = 1;
            dRange = lpth->thp().iWarp() + 4;
            dRange *= dRange;
            if (fPostProd) {
                dRange >>= 1;
            }
            ptDst = app.state.rgptPlan[lpth->thp().idPlanet()];
        MoveTh:
            ptSrc = lpth->pt();
            d = DGetDistance(ptSrc.x, ptSrc.y, ptDst.x, ptDst.y);
            dLeft = LOWORD(static_cast<std::int32_t>(d));
            if (dLeft <= dRange) {
            MadeItThere:
                if (lpth->ith() == ithMysteryTrader) {
                    lpth2 = app.state.lpThings;
                    lpth2Mac = app.state.lpThings + app.state.cThing;
                    for (; lpth2 < lpth2Mac && (lpth2->ith() != ithMysteryTrader || lpth2 == lpth); lpth2++) {
                    }
                    if (lpth2 != lpth2Mac || RandomIO().pfnRandom(app, 2) == 0)
                        goto LFreeThePacket;
                    { const POINT16 ptForThing = (lpth->tht().ptDest()); auto &thForThing = *(lpth); thForThing.pt().Set(ptForThing); }
                    dRange = lpth->tht().iWarp() - 2;
                    if (dRange < 6) {
                        dRange = 6;
                    }
                    idm = idmMysteryTraderHasDecidedMakeAnotherPass;
                    goto LRetargetFreighter;
                }
                if (lpth->ith() == ithMineralPacket) {
                    pctRate = LMulDiv(dLeft, 100, dRange);
                    if (pctRate < 0) {
                        pctRate = 0;
                    } else if (pctRate > 100) {
                        pctRate = 100;
                    }
                    if (fPostProd) {
                        pctRate >>= 1;
                    }
                    if (FPacketDecay(app, lpth, pctRate))
                        goto LPacketAlreadyFreed;
                }
                lppl = app.state.lpPlanets + lpth->thp().idPlanet();
                iWarpPacket = lpth->thp().iWarp() + 4;
                iWarp = ColonyIO().pfnWarp(app, lppl, &fTwoMAs);
                if (fTwoMAs) {
                    iWarp++;
                }
                if (iWarp > 0 && RaceQueryIO().pfnStat(&app.state.rgplr[lpth->iplr()], rsMajorAdv) == raMassAccel) {
                    app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].grbitPlr = app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].grbitPlr | 1 << lpth->iplr();
                }
                fTerra = RaceQueryIO().pfnStat(&app.state.rgplr[lpth->iplr()], rsMajorAdv) == raMassAccel;
                iWarp2 = iWarp * iWarp;
                iWarpPacket2 = iWarpPacket * iWarpPacket;
                if (lppl->iPlayer != iplrNone && RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv) == raStargate) {
                    iWarp2 /= 2;
                }
                if (iWarp2 >= iWarpPacket2) {
                    pctCaught = 1000;
                } else if (iWarp > 0) {
                    iWarp = iWarp2;
                    pctCaught = LOWORD(static_cast<std::int32_t>((static_cast<std::int32_t>((iWarp * 1000)) / iWarpPacket2)));
                } else {
                    pctCaught = 0;
                }
                pctMinKeep = 1000 - pctCaught;
                for (i = 0; i < 3; i++) {
                    rgwtTerra[i] = LOWORD(static_cast<std::int32_t>((lpth->thp().rgwtMin(i) * pctMinKeep)) / 1000);
                }
                pctMinKeep = static_cast<std::int16_t>((1000 - pctCaught)) / 9 + pctCaught;
                wtTot = 0;
                for (i = 0; i < 3; i++) {
                    if (lpth->thp().rgwtMin(i) < 0) {
                        lpth->thp().SetRgwtMin(i, 0);
                    }
                    wtTot += lpth->thp().rgwtMin(i);
                    lppl->rgwtMin[i] += static_cast<std::int32_t>((lpth->thp().rgwtMin(i) * pctMinKeep)) / 1000;
                }
                if (pctCaught == 1000)
                    goto LAllSafe;
                dmgRaw = static_cast<std::int32_t>((static_cast<std::int16_t>((iWarpPacket * iWarpPacket - iWarp)) * wtTot)) / 160;
                if (fTerra) {
                    iplr = lpth->iplr();
                    for (i = 0; i < 3; i++) {
                        cTerraTemp = 0;
                        cTerraPerm = 0;
                        while (rgwtTerra[i] > 0) {
                            wtCur = rgwtTerra[i] >= 100 ? 100 : rgwtTerra[i];
                            if (RandomIO().pfnRandom(app, 200) < wtCur) {
                                cTerraTemp++;
                                if (RandomIO().pfnRandom(app, 10) == 0) {
                                    cTerraPerm++;
                                }
                            }
                            rgwtTerra[i] -= 100;
                        }
                        if (cTerraPerm > 0) {
                            if (app.state.rgplr[iplr].rgEnvVarMin[i] < 0) {
                                if (lppl->rgEnvVarOrig[i] < 50) {
                                    cTerraPerm = -(cTerraPerm >= lppl->rgEnvVarOrig[i] - 1 ? lppl->rgEnvVarOrig[i] - 1 : cTerraPerm);
                                } else {
                                    cTerraPerm = cTerraPerm >= 99 - lppl->rgEnvVarOrig[i] ? 99 - lppl->rgEnvVarOrig[i] : cTerraPerm;
                                }
                            } else if (lppl->rgEnvVarOrig[i] < app.state.rgplr[iplr].rgEnvVar[i]) {
                                if (lppl->rgEnvVarOrig[i] + cTerraPerm > app.state.rgplr[iplr].rgEnvVar[i]) {
                                    cTerraPerm = app.state.rgplr[iplr].rgEnvVar[i] - lppl->rgEnvVarOrig[i];
                                }
                            } else if (lppl->rgEnvVarOrig[i] <= app.state.rgplr[iplr].rgEnvVar[i]) {
                                cTerraPerm = 0;
                            } else if (lppl->rgEnvVarOrig[i] - cTerraPerm < app.state.rgplr[iplr].rgEnvVar[i]) {
                                cTerraPerm = app.state.rgplr[iplr].rgEnvVar[i] - lppl->rgEnvVarOrig[i];
                            } else {
                                cTerraPerm = -cTerraPerm;
                            }
                            if (cTerraPerm != 0) {
                                TurnIO().pfnMessage(app, iplr, idmMineralPacketHasPermanentlyDefault, static_cast<MsgGoto>(lppl->id), cTerraPerm > 0, i, lppl->id, std::abs(cTerraPerm), 0, 0, 0);
                                if (lppl->iPlayer != iplrNone && lppl->iPlayer != iplr) {
                                    TurnIO().pfnMessage(app, iplr, idmMineralPacketHasPermanentlyDefault2, static_cast<MsgGoto>(lppl->id), cTerraPerm > 0, i, lppl->id, std::abs(cTerraPerm), 0, 0, 0);
                                }
                                lppl->rgEnvVarOrig[i] += cTerraPerm;
                            }
                        }
                        if (cTerraTemp > 0) {
                            app.state.idPlayer = iplr;
                            if (!ColonyIO().pfnCanTerraform(app, lppl, rgMin, rgMax, rgCost, 1)) {
                                app.state.idPlayer = iplrNone;
                            } else {
                                app.state.idPlayer = iplrNone;
                                if (app.state.rgplr[iplr].rgEnvVarMin[i] < 0) {
                                    cTerraTemp /= 2;
                                    if (lppl->rgEnvVarOrig[i] < 50) {
                                        cTerraTemp = -(cTerraTemp >= lppl->rgEnvVar[i] - 1 ? lppl->rgEnvVar[i] - 1 : cTerraTemp);
                                    } else {
                                        cTerraTemp = cTerraTemp >= 99 - lppl->rgEnvVar[i] ? 99 - lppl->rgEnvVar[i] : cTerraTemp;
                                    }
                                } else if (rgMin[i] != envNone) {
                                    if (cTerraTemp > lppl->rgEnvVar[i] - rgMin[i]) {
                                        cTerraTemp = rgMin[i] - lppl->rgEnvVar[i];
                                    } else {
                                        cTerraTemp = -cTerraTemp;
                                    }
                                } else if (rgMax[i] == envNone) {
                                    cTerraTemp = 0;
                                } else if (cTerraTemp > rgMax[i] - lppl->rgEnvVar[i]) {
                                    cTerraTemp = rgMax[i] - lppl->rgEnvVar[i];
                                }
                                if (cTerraTemp != 0) {
                                    lppl->rgEnvVar[i] += cTerraTemp;
                                    TurnIO().pfnMessage(app, iplr, idmMineralPacketHas, static_cast<MsgGoto>(lppl->id), cTerraTemp > 0, i, lppl->id, i << 8 | lppl->rgEnvVar[i], 0, 0, 0);
                                    if (lppl->iPlayer != iplrNone && lppl->iPlayer != iplr) {
                                        TurnIO().pfnMessage(app, iplr, idmMineralPacketHas2, static_cast<MsgGoto>(lppl->id), cTerraTemp > 0, i, lppl->id, i << 8 | lppl->rgEnvVar[i], 0, 0, 0);
                                    }
                                }
                            }
                        }
                    }
                }
                if (lppl->iPlayer == iplrNone)
                    goto LFreeThePacket;
                CalcPctSurvive(app, lppl, &pct, nullptr);
                dmgRaw = static_cast<std::int32_t>((static_cast<double>(pct) * dmgRaw));
                if (dmgRaw == 0 || RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv) == raMacintosh) {
                LAllSafe:
                    TurnIO().pfnMessage(app, lppl->iPlayer,
                                iWarp <= 0 ? idmBombardedPacketContainingKtMineralsHoweverPacket : idmMassAcceleratorHasSuccessfullyCapturedPacketCont,
                                static_cast<MsgGoto>(lppl->id), lppl->id, lpth->iplr(), LOWORD(wtTot), HIWORD(wtTot), 0, 0, 0);
                    goto LFreeThePacket;
                }
                lColKilled = lppl->rgwtMin[3];
                if (lColKilled != 0) {
                    lColKilled = static_cast<std::int32_t>((lColKilled * dmgRaw)) / 1000;
                    if (lColKilled < dmgRaw) {
                        lColKilled = dmgRaw;
                    }
                    if (lppl->rgwtMin[3] > 0 && (lColKilled >= lppl->rgwtMin[3] || lColKilled < 0)) {
                        TurnIO().pfnMessage2(app, lppl->iPlayer, idmAnnihilatedMineralPacketColonistsKilled, static_cast<MsgGoto>(lppl->id), lppl->id, lpth->iplr());
                        UninhabitPlanet(app, lppl);
                        goto LFreeThePacket;
                    }
                    lDefKilled = static_cast<std::int32_t>((lppl->cDefenses * dmgRaw)) / 1000;
                    if (lDefKilled == 0 && lppl->cDefenses != 0) {
                        lDefKilled = static_cast<std::uint32_t>((RandomIO().pfnRandom(app, 20) < dmgRaw));
                    }
                    if (lDefKilled < static_cast<std::int32_t>((dmgRaw / 20))) {
                        lDefKilled = static_cast<std::int32_t>((dmgRaw / 20));
                    }
                    if (lDefKilled > static_cast<std::int32_t>(lppl->cDefenses)) {
                        lDefKilled = lppl->cDefenses;
                    }
                    if (lDefKilled == 0) {
                        idm = iWarp == 0 ? idmBombardedKtMineralPacketColonistsKilledCollision : idmMassAcceleratorPartiallySuccessfullyCapturingKtM;
                        TurnIO().pfnMessage(app, lppl->iPlayer, idm, static_cast<MsgGoto>(lppl->id), lppl->id, LOWORD(wtTot), HIWORD(wtTot), lpth->iplr(), LOWORD(lColKilled), 0, 0);
                    } else {
                        idm = iWarp == 0 ? idmBombardedKtMineralPacketColonistsDefensesDestroy : idmMassAcceleratorPartiallySuccessfullyCapturingKtM2;
                        TurnIO().pfnMessage(app, lppl->iPlayer, idm, static_cast<MsgGoto>(lppl->id), lppl->id, LOWORD(wtTot), HIWORD(wtTot), lpth->iplr(), LOWORD(lColKilled), LOWORD(lDefKilled),
                                    0);
                        lppl->cDefenses -= LOWORD(lDefKilled);
                    }
                } else {
                    TurnIO().pfnMessage2(app, lppl->iPlayer, idmBombardedKtMineralPacketFortunatelyOneHome, static_cast<MsgGoto>(lppl->id), lppl->id, lpth->iplr());
                    lDefKilled = lppl->cDefenses;
                    lColKilled = 0;
                }
                lppl->rgwtMin[3] -= lColKilled;
            LFreeThePacket:
                FreeLpth(app, lpth);
            LPacketAlreadyFreed:
                lpth--;
                lpthMac--;
            } else {
                dxRound = static_cast<double>((ptDst.x <= ptSrc.x ? static_cast<double>(-0.5) : static_cast<double>(0.5)));
                dyRound = static_cast<double>((ptDst.y <= ptSrc.y ? static_cast<double>(-0.5) : static_cast<double>(0.5)));
                if (static_cast<double>(d) > static_cast<double>(0.0001) || static_cast<double>(d) < static_cast<double>(-0.0001)) {
                    r = static_cast<double>((static_cast<double>(dRange) / d));
                    ptSrc.x = LOWORD(static_cast<std::int32_t>((static_cast<double>(r) * static_cast<std::int16_t>((ptDst.x - ptSrc.x)) + dxRound))) + ptSrc.x;
                    ptSrc.y = LOWORD(static_cast<std::int32_t>((static_cast<double>(r) * static_cast<std::int16_t>((ptDst.y - ptSrc.y)) + dyRound))) + ptSrc.y;
                    if (ptSrc.x == ptDst.x && ptSrc.y == ptDst.y)
                        goto MadeItThere;
                    lpth->pt().Set(ptSrc);
                }
                if (fPostProd && lpth->ith() == ithMineralPacket && FPacketDecay(app, lpth, 50))
                    goto LPacketAlreadyFreed;
            }
        }
    }
    if (fAnythingMoved) {
        ValidateWaypoints(app);
    }
    return;
}

void FuelFleets(APPSESSION &app) {
    std::int16_t j;
    std::int32_t cPods;
    PLANET *lppl;
    std::int16_t i;
    std::int16_t ifl;
    FLEET  *lpfl;
    SHDEF  *lpshdef;
    std::int32_t csh;
    HUL    *lphul;

    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->fDead)
            continue;
        if (lpfl->idPlanet == idPlanetDeepSpace || !app.state.lpPlanets[lpfl->idPlanet].fStarbase)
            goto LChkFuelTransport;
        lppl = app.state.lpPlanets + lpfl->idPlanet;
        if (lppl->iPlayer == iplrNone)
            goto LChkFuelTransport;
        if (lpfl->iPlayer != lppl->iPlayer && app.state.rgplr[lppl->iPlayer].rgmdRelation[lpfl->iPlayer] != 1)
            goto LChkFuelTransport;
        if (PartQueryIO().pfnHull(app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].hul.ihuldef)->hul.wtCargoMax == 0)
            goto LChkFuelTransport;
        lpfl->rgwtMin[4] = ProductionIO().pfnFleetStat(app, lpfl, 1);
        continue;

    LChkFuelTransport:
        csh = 0;
        cPods = 0;
        for (i = 0; i < 16; i++) {
            if (lpfl->rgcsh[i] != 0) {
                lphul = &app.state.rglpshdef[lpfl->iPlayer][i].hul;
                for (j = lphul->chs - 1; j >= 0; j--) {
                    if (lphul->rghs[j].grhst == hstSpecialE && lphul->rghs[j].iItem == ispecialEAntiMatterGenerator) {
                        cPods += static_cast<std::uint32_t>((lpfl->rgcsh[i] * lphul->rghs[j].cItem));
                    }
                }
                lpshdef = app.state.rglpshdef[lpfl->iPlayer] + i;
                if (lpshdef->hul.ihuldef == ihuldefFuelTransport || lpshdef->hul.ihuldef == ihuldefSuperFuelXport) {
                    csh += static_cast<std::uint32_t>((lpfl->rgcsh[i] * 200));
                }
            }
        }
        if (csh != 0 || cPods != 0) {
            lpfl->rgwtMin[4] = (((ProductionIO().pfnFleetStat(app, lpfl, 1)) < (lpfl->rgwtMin[4] + csh + static_cast<std::uint32_t>((cPods * 50)))) ? (ProductionIO().pfnFleetStat(app, lpfl, 1)) : (lpfl->rgwtMin[4] + csh + static_cast<std::uint32_t>((cPods * 50))));
        }
    }
    return;
}

void MoveFleets(APPSESSION &app) {
    std::int32_t dTravel;
    std::int16_t cPass;
    std::int32_t wtFuel2Dest;
    double  d;
    std::int16_t fGotEnufFuel;
    std::int16_t fRanOutOfFuel;
    ORDER  *lpord;
    POINT16 ptEnd;
    std::int16_t ifl;
    FLEET  *lpfl;
    double  r;
    std::int32_t pct;
    std::int16_t dMineTravel;
    std::int32_t dRange;
    POINT16 ptBeg;
    std::int32_t wtFuelUsed;
    std::int32_t dActTravel;
    std::int32_t lFuelGain;
    std::int16_t fDone;
    SCAN    scan;
    PLANET *lpplDst;
    std::int32_t wtColonists;
    std::int16_t i;
    PLANET *lpplSrc;
    std::int16_t fJumpgate;
    std::int16_t isbsDst;
    std::int16_t isbsSrc;
    POINT16 ptMsg;
    std::int32_t wtMinerals;
    std::int32_t cDie;
    std::int16_t cKill;
    std::int16_t ish;
    FLEET   flSrc;
    std::int16_t cTry;
    FLEET   flDead;
    std::int16_t cKillTot;
    std::int16_t fDead;
    std::int16_t dy;
    std::int16_t dx;
    std::int32_t lFuelGainAct;
    double  dyRound;
    double  dxRound;
    std::int16_t iCtr;
    THING  *lpthDest;
    THING  *lpth;
    std::int16_t grbitPlr;

    cPass = 0;
    if (app.state.cFleet > 0) {
    MoveUnfinishedFleets:
        fDone = 1;
        for (ifl = 0; ifl < app.state.cFleet; ifl++) {
            lpfl = app.state.rglpfl[ifl];
            if (!app.state.rglpfl[ifl])
                break;
            if (cPass == 0) {
                lpfl->dirLong = 0;
                lpfl->fHereAllTurn = 1;
            }
            if (!lpfl->fDead && (cPass <= 0 || !lpfl->fDone)) {
                lpfl->fDone = 1;
                lpord = lpfl->lpplord->rgord();
                if (lpord->grTask != grTaskXfer && lpord->grTask != grTaskLayMines && lpfl->cord > 1 && lpord[1].iWarp != 0) {
                    if (cPass == 0 && lpord[1].iWarp > 6 && lpord[1].iWarp != 11 && RaceQueryIO().pfnGrbit(&app.state.rgplr[lpfl->iPlayer], ibitRaceCheapEngines) != 0 &&
                        RandomIO().pfnRandom(app, 10) == 0) {
                        TurnIO().pfnMessage2(app, lpfl->iPlayer, idmUnableEngageEnginesDueBalkyEquipmentEngineers, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                    } else {
                        if (lpord[1].iWarp >= 11) {
                            fJumpgate = 0;
                            app.state.gd.fRadiatingEngine = 0;
                            ptMsg = lpord->pt;
                            ptBeg = lpord->pt;
                            if (lpord->grobj == grobjPlanet) {
                                ptMsg.x = -1;
                                ptMsg.y = lpord->id;
                                lpplSrc = LpplFromId(app, lpord->id);
                                isbsSrc = IStargateFromLppl(app, lpplSrc);
                            } else {
                                isbsSrc = -1;
                            }
                            if (isbsSrc == -1) {
                                if (FFleetCanJumpgate(app, lpfl))
                                    goto LNoGateNeeded;
                                TurnIO().pfnMessage(app, lpfl->iPlayer, idmAttemptedUseStargateStargateExistsThere, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, ptMsg.x, ptMsg.y, 0, 0, 0,
                                            0);
                                continue;
                            }
                            if (lpplSrc->iPlayer != lpfl->iPlayer && app.state.rgplr[lpplSrc->iPlayer].rgmdRelation[lpfl->iPlayer] != 1) {
                                TurnIO().pfnMessage(app, lpfl->iPlayer, idmAttemptedUseStargateCouldBecauseStarbaseOwned, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lpplSrc->id,
                                            lpplSrc->id, 0, 0, 0, 0);
                                continue;
                            }
                        LNoGateNeeded:
                            ptMsg = lpord[1].pt;
                            ptEnd = lpord[1].pt;
                            if (lpord[1].grobj == grobjPlanet) {
                                ptMsg.x = -1;
                                ptMsg.y = lpord[1].id;
                                lpplDst = LpplFromId(app, lpord[1].id);
                                isbsDst = IStargateFromLppl(app, lpplDst);
                            } else {
                                for (i = 0; i < app.state.game.cPlanMax && (ptEnd.x != app.state.rgptPlan[i].x || ptEnd.y != app.state.rgptPlan[i].y); i++) {
                                }
                                if (i < app.state.game.cPlanMax) {
                                    ptMsg.x = -1;
                                    ptMsg.y = i;
                                    lpplDst = LpplFromId(app, i);
                                    isbsDst = IStargateFromLppl(app, lpplDst);
                                } else {
                                    TurnIO().pfnMessage(app, lpfl->iPlayer, idmAttemptedReachViaStargateCouldBecauseStargate, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, ptEnd.x, ptEnd.y,
                                                0, 0, 0, 0);
                                    continue;
                                }
                            }
                            if (isbsDst == -1) {
                                TurnIO().pfnMessage(app, lpfl->iPlayer, idmAttemptedUseStargateReachCouldBecauseStargate, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lpplDst->id, ptMsg.x,
                                            ptMsg.y, 0, 0, 0);
                                continue;
                            }
                            if (lpplDst->iPlayer != lpfl->iPlayer && app.state.rgplr[lpplDst->iPlayer].rgmdRelation[lpfl->iPlayer] != 1) {
                                TurnIO().pfnMessage(app, lpfl->iPlayer, idmAttemptedUseStargateReachCouldBecauseStarbase, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lpplDst->id,
                                            lpplDst->id, lpplDst->id, 0, 0, 0);
                                continue;
                            }
                            if (isbsSrc == -1) {
                                fJumpgate = 1;
                                isbsSrc = isbsDst;
                            }
                            if (!fJumpgate && RaceQueryIO().pfnStat(&app.state.rgplr[lpfl->iPlayer], rsMajorAdv) != raStargate) {
                                if (lpfl->rgwtMin[3] > 0 && lpplSrc->iPlayer != lpfl->iPlayer) {
                                    TurnIO().pfnMessage2(app, lpfl->iPlayer, idmUnableUseStargateBecauseHadColonistsBoard, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lpplSrc->id);
                                    continue;
                                }
                                wtMinerals = 0;
                                for (i = 0; i <= 2; i++) {
                                    if (lpfl->rgwtMin[i] != 0) {
                                        wtMinerals += lpfl->rgwtMin[i];
                                        lpplSrc->rgwtMin[i] += lpfl->rgwtMin[i];
                                        lpfl->rgwtMin[i] = 0;
                                    }
                                }
                                wtColonists = lpfl->rgwtMin[3];
                                lpplSrc->rgwtMin[3] += lpfl->rgwtMin[3];
                                lpfl->rgwtMin[3] = 0;
                                if (wtColonists != 0) {
                                    if (wtMinerals != 0) {
                                        TurnIO().pfnMessage(app, lpfl->iPlayer, idmHasUnloadedColonistsKtMineralsPreparationJumping, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id,
                                                    LOWORD(wtColonists), HIWORD(wtColonists), LOWORD(wtMinerals), HIWORD(wtMinerals), lpplSrc->id, 0);
                                        if (lpfl->iPlayer != lpplSrc->iPlayer) {
                                            TurnIO().pfnMessage(app, lpplSrc->iPlayer, idmHasUnloadedColonistsKtMineralsPreparationJumping, static_cast<MsgGoto>(lpplSrc->id), lpfl->id,
                                                        LOWORD(wtColonists), HIWORD(wtColonists), LOWORD(wtMinerals), HIWORD(wtMinerals), lpplSrc->id, 0);
                                        }
                                    } else {
                                        TurnIO().pfnMessage(app, lpfl->iPlayer, idmHasUnloadedColonistsPreparationJumpingThroughSta, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id,
                                                    LOWORD(wtColonists), HIWORD(wtColonists), lpplSrc->id, 0, 0, 0);
                                        if (lpfl->iPlayer != lpplSrc->iPlayer) {
                                            TurnIO().pfnMessage(app, lpplSrc->iPlayer, idmHasUnloadedColonistsPreparationJumpingThroughSta, static_cast<MsgGoto>(lpplSrc->id), lpfl->id,
                                                        LOWORD(wtColonists), HIWORD(wtColonists), lpplSrc->id, 0, 0, 0);
                                        }
                                    }
                                } else if (wtMinerals != 0) {
                                    TurnIO().pfnMessage(app, lpfl->iPlayer, idmHasUnloadedKtMineralsPreparationJumpingThrough, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id,
                                                LOWORD(wtMinerals), HIWORD(wtMinerals), lpplSrc->id, 0, 0, 0);
                                    if (lpfl->iPlayer != lpplSrc->iPlayer) {
                                        TurnIO().pfnMessage(app, lpplSrc->iPlayer, idmHasUnloadedKtMineralsPreparationJumpingThrough, static_cast<MsgGoto>(lpplSrc->id), lpfl->id,
                                                    LOWORD(wtMinerals), HIWORD(wtMinerals), lpplSrc->id, 0, 0, 0);
                                    }
                                }
                            }
                            dTravel = static_cast<std::int32_t>(DGetDistance(ptBeg.x, ptBeg.y, ptEnd.x, ptEnd.y));
                            if (!FStargateJump(app, lpfl, isbsSrc, isbsDst, LOWORD(dTravel)))
                                continue;
                            lpfl->fHereAllTurn = 0;
                            NoAutoTrackFleet(app, lpfl);
                            goto LMakeItToDest;
                        } else {
                            ptBeg = lpfl->pt;
                            if (cPass > 0 && !lpord[1].fNoAutoTrack) {
                                lpord[1].pt = lpfl->lpflNext->pt;
                            }
                            ptEnd = lpord[1].pt;
                            dRange = EstFuelUse(app, lpfl, 0, -1, -1, 1);
                            wtFuel2Dest = EstFuelUse(app, lpfl, 0, -1, -1, 0);
                            fGotEnufFuel = wtFuel2Dest <= lpfl->rgwtMin[4];
                            fRanOutOfFuel = 0;
                            if (fGotEnufFuel) {
                                dRange = dRange <= static_cast<std::int32_t>(static_cast<std::uint32_t>((lpord[1].iWarp * lpord[1].iWarp))) ? static_cast<std::uint32_t>((lpord[1].iWarp * lpord[1].iWarp)) : dRange;
                            }
                            if (cPass == 0) {
                                if (lpfl->rgwtMin[3] > 10 && RaceQueryIO().pfnStat(&app.state.rgplr[lpfl->iPlayer], rsMajorAdv) == raMacintosh) {
                                    cDie = static_cast<std::int32_t>((static_cast<std::uint32_t>((lpfl->rgwtMin[3] * 3)) + 33)) / 100;
                                    if (cDie > 0) {
                                        lpfl->rgwtMin[3] -= cDie;
                                        TurnIO().pfnMessage(app, lpfl->iPlayer, idmDueRigorsWarpAccelerationColonistsHaveDied, static_cast<MsgGoto>(lpfl->id | 0x8000), LOWORD(cDie), HIWORD(cDie),
                                                    lpfl->id, 0, 0, 0, 0);
                                    }
                                }
                                if (lpord[1].iWarp == 10) {
                                    flSrc = *lpfl;
                                    fDead = 1;
                                    cKillTot = 0;
                                    flDead = {};
                                    for (ish = 0; ish < 16; ish++) {
                                        if (flSrc.rgcsh[ish] != 0) {
                                            if (app.state.rglpshdef[lpfl->iPlayer][ish].hul.rghs[0].iItem == iengineInterspace10 ||
                                                app.state.rglpshdef[lpfl->iPlayer][ish].hul.rghs[0].iItem == iengineTransStar10 ||
                                                app.state.rglpshdef[lpfl->iPlayer][ish].hul.rghs[0].iItem == iengineTransGalacticMizerScoop ||
                                                app.state.rglpshdef[lpfl->iPlayer][ish].hul.rghs[0].iItem == iengineGalaxyScoop ||
                                                app.state.rglpshdef[lpfl->iPlayer][ish].hul.rghs[0].iItem == iengineEnigmaPulsar)
                                                goto LWarp10Kill;
                                            cKill = 0;
                                            cTry = flSrc.rgcsh[ish];
                                            while (cTry-- != 0) {
                                                if (RandomIO().pfnRandom(app, 10) == 0) {
                                                    cKill++;
                                                }
                                            }
                                            if (cKill > 0) {
                                                cKillTot += cKill;
                                                flSrc.rgcsh[ish] -= cKill;
                                                flDead.rgcsh[ish] = cKill;
                                            }
                                        LWarp10Kill:
                                            if (flSrc.rgcsh[ish] > 0) {
                                                fDead = 0;
                                            }
                                        }
                                    }
                                    if (fDead) {
                                        lpfl->fDead = 1;
                                        TurnIO().pfnMessage2(app, lpfl->iPlayer, idmDestroyedMassiveReactorAccidentDueUnsafeOperatin, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                                        continue;
                                    }
                                    if (cKillTot > 0) {
                                        flDead.iPlayer = flSrc.iPlayer;
                                        flDead.fDead = 1;
                                        flDead.det = detAll;
                                        FleetTransferCargoBalance(app, &flSrc, &flDead);
                                        *lpfl = flSrc;
                                        if (cKillTot == 1) {
                                            TurnIO().pfnMessage2(app, lpfl->iPlayer, idmOneShipsDestroyedWhenEnginesReactedTrying, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                                        } else {
                                            TurnIO().pfnMessage2(app, lpfl->iPlayer, idmShipsDestroyedDueEngineStrain, static_cast<MsgGoto>(lpfl->id | 0x8000), cKillTot, lpfl->id);
                                        }
                                    }
                                }
                                dTravel = static_cast<std::uint32_t>((lpord[1].iWarp * lpord[1].iWarp));
                                if (lpord[1].grobj == grobjFleet) {
                                    lpfl->lpflNext = LpflFromId(app, lpord[1].id);
                                    if (lpfl->lpflNext) {
                                        fDone = 0;
                                        lpfl->fDone = 0;
                                        lpfl->lPower = static_cast<std::uint32_t>(LOWORD(dTravel));
                                        lpfl->lFuelUsed = 0;
                                        continue;
                                    }
                                }
                            } else {
                                if (lpfl->lpflNext->fDone) {
                                    dTravel = lpfl->dMoveLeft();
                                } else {
                                    dTravel = lpfl->dMoveLeft() >= static_cast<std::int16_t>((lpfl->dMoveLeft() + lpfl->dMoveUsed() + 4)) / 5
                                                  ? static_cast<std::int16_t>((static_cast<std::int16_t>((lpfl->dMoveLeft() + lpfl->dMoveUsed() + 4)) / 5))
                                                  : lpfl->dMoveLeft();
                                }
                                dRange -= lpfl->dMoveUsed();
                                if (dRange < 0) {
                                    dRange = 0;
                                }
                            }
                            d = DGetDistance(ptBeg.x, ptBeg.y, ptEnd.x, ptEnd.y);
                            dTravel =
                                dTravel < static_cast<std::int16_t>(LOWORD(static_cast<std::int32_t>((static_cast<double>(d) + 0.9999)))) ? dTravel : static_cast<std::int16_t>(LOWORD(static_cast<std::int32_t>((static_cast<double>(d) + 0.9999))));
                            if (dTravel > dRange) {
                                lpfl->rgwtMin[4] = 0;
                                wtFuelUsed = 1;
                                dTravel = dRange;
                            } else {
                                if (cPass > 0) {
                                    lpfl->rgwtMin[4] += lpfl->lFuelUsed;
                                    dTravel += lpfl->dMoveUsed();
                                }
                                wtFuelUsed = EstFuelUse(app, lpfl, 0, -1, dTravel, 0);
                                if (cPass > 0) {
                                    lpfl->lFuelUsed = wtFuelUsed;
                                    dTravel -= lpfl->dMoveUsed();
                                }
                                lpfl->rgwtMin[4] = 0 <= lpfl->rgwtMin[4] - wtFuelUsed ? lpfl->rgwtMin[4] - wtFuelUsed : 0;
                            }
                            if (lpfl->rgwtMin[4] == 0 && wtFuelUsed > 0 &&
                                ((static_cast<double>(d) - 0.99999 >= static_cast<double>(dTravel) || dRange == 0) && !fGotEnufFuel)) {
                                i = 0;
                                do {
                                    i++;
                                } while (EstFuelUse(app, lpfl, 0, i, -1, 0) == 0 && i < 10);
                                if (i > 1) {
                                    lpfl->lpplord->rgord()[1].iWarp = i - 1;
                                    TurnIO().pfnMessage2(app, lpfl->iPlayer, idmHasRunFuelFleetsSpeedHasDecreased, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, i - 1);
                                } else {
                                    TurnIO().pfnMessage2(app, lpfl->iPlayer, idmHasRunFuel, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                                }
                                fRanOutOfFuel = 1;
                            }
                            if (dRange == 0)
                                continue;
                            lpfl->fHereAllTurn = 0;
                            dx = ptEnd.x - ptBeg.x;
                            dy = ptEnd.y - ptBeg.y;
                            if (dx != 0 || dy != 0) {
                                lpfl->SetFdirValid(1);
                                for (; std::abs(dx) > 127 || std::abs(dy) > 127; dy /= 2) {
                                    dx /= 2;
                                }
                                lpfl->SetDirFltX(dx + 127);
                                lpfl->SetDirFltY(dy + 127);
                                lpfl->SetIwarpFlt(lpfl->lpplord->rgord()[1].iWarp);
                            }
                            dActTravel = static_cast<std::int32_t>((static_cast<double>(d) - 0.99999));
                            dMineTravel = dTravel < dActTravel ? LOWORD(dTravel) : LOWORD(dActTravel);
                            if (lpord[1].iWarp < 11 && !FTravelThroughMineFields(app, lpfl, &dMineTravel, nullptr)) {
                                lpfl->SetDMoveLeft(0);
                                if (lpfl->fDead)
                                    continue;
                                if (dMineTravel < dActTravel) {
                                    dTravel = dMineTravel;
                                }
                            } else if (!fRanOutOfFuel && GetFuelFree(app, lpfl) > 0) {
                                lFuelGain = LCalcFuelGainFromRamScoops(app, lpfl, lpord[1].iWarp, dTravel < dActTravel ? dTravel : dActTravel);
                                if (lFuelGain > 0) {
                                    lFuelGainAct = ChgCargo(app, grobjFleet, lpfl->id, Fuel, lFuelGain, nullptr);
                                    if (lFuelGain > 32500) {
                                        lFuelGain = 32500;
                                    }
                                    TurnIO().pfnMessage2(app, lpfl->iPlayer, idmSRamScoopsHaveProducedMgFuel, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, LOWORD(lFuelGain));
                                }
                            }
                            if (dActTravel < dTravel || dActTravel <= 0) {
                            LMakeItToDest:
                                lpfl->pt = ptEnd;
                                if (lpord[1].grobj == grobjPlanet) {
                                    lpfl->idPlanet = lpord[1].id;
                                } else {
                                    lpfl->idPlanet = idPlanetDeepSpace;
                                }
                                /* A pursuer that catches a fleet still on its
                                   own pursuit keeps following it with the rest
                                   of its move. The original marked the caught
                                   fleet done instead, which stopped it short of
                                   its own target. */
                                if (cPass > 0 && !lpfl->lpflNext->fDone) {
                                    lpfl->SetDMoveUsed(lpfl->dMoveUsed() + (LOWORD(dTravel)));
                                    lpfl->SetDMoveLeft(lpfl->dMoveLeft() - (LOWORD(dTravel)));
                                    if (lpfl->dMoveLeft() > 0 && !fRanOutOfFuel) {
                                        fDone = 0;
                                        lpfl->fDone = 0;
                                    }
                                }
                            } else {
                                dxRound = static_cast<double>((ptEnd.x <= ptBeg.x ? static_cast<double>(-0.5) : static_cast<double>(0.5)));
                                dyRound = static_cast<double>((ptEnd.y <= ptBeg.y ? static_cast<double>(-0.5) : static_cast<double>(0.5)));
                                if (static_cast<double>(d) > static_cast<double>(0.0001) || static_cast<double>(d) < static_cast<double>(-0.0001)) {
                                    r = static_cast<double>((static_cast<double>(dTravel) / d));
                                    lpfl->pt.x = LOWORD(static_cast<std::int32_t>((static_cast<double>(r) * static_cast<std::int16_t>((ptEnd.x - ptBeg.x)) + dxRound))) + ptBeg.x;
                                    lpfl->pt.y = LOWORD(static_cast<std::int32_t>((static_cast<double>(r) * static_cast<std::int16_t>((ptEnd.y - ptBeg.y)) + dyRound))) + ptBeg.y;
                                    lpfl->idPlanet = idPlanetDeepSpace;
                                }
                                if (cPass > 0 && lpfl->dMoveLeft() > 0) {
                                    lpfl->SetDMoveUsed(lpfl->dMoveUsed() + (LOWORD(dTravel)));
                                    lpfl->SetDMoveLeft(lpfl->dMoveLeft() - (LOWORD(dTravel)));
                                    if (lpfl->dMoveLeft() > 0 && !fRanOutOfFuel) {
                                        fDone = 0;
                                        lpfl->fDone = 0;
                                    }
                                }
                            }
                        }
                        if (app.state.gd.fRadiatingEngine && lpfl->rgwtMin[3] > 0 && cPass <= 1 &&
                            app.state.rgplr[lpfl->iPlayer].rgEnvVarMin[2] + app.state.rgplr[lpfl->iPlayer].rgEnvVarMax[2] < 170 &&
                            app.state.rgplr[lpfl->iPlayer].rgEnvVarMax[2] != envImmune) {
                            iCtr = static_cast<std::int16_t>((app.state.rgplr[lpfl->iPlayer].rgEnvVarMin[2] + app.state.rgplr[lpfl->iPlayer].rgEnvVarMax[2])) / 2;
                            pct = static_cast<std::int16_t>(((0x56 - iCtr) >> 1));
                            pct = static_cast<std::int32_t>((pct * lpfl->rgwtMin[3])) / 100;
                            if (lpfl->rgwtMin[3] < static_cast<std::int16_t>((1 <= pct ? pct : 1))) {
                                pct = lpfl->rgwtMin[3];
                            } else if (1 > pct) {
                                pct = 1;
                            }
                            TurnIO().pfnMessage2(app, lpfl->iPlayer, idmEngineRadiationHasKilledColonistsTraveling, static_cast<MsgGoto>(lpfl->id | 0x8000), LOWORD(pct), lpfl->id);
                            lpfl->rgwtMin[3] -= pct;
                        }
                        if (ptEnd.x == lpfl->pt.x && ptEnd.y == lpfl->pt.y && lpord[1].grobj == grobjThing) {
                            lpth = LpthFromId(app, lpord[1].id);
                            if (lpth && lpth->ith() == ithWormhole) {
                                grbitPlr = 1 << lpfl->iPlayer;
                                lpthDest = LpthFromId(app, lpth->thw().idPartner());
                                NoAutoTrackFleet(app, lpfl);
                                lpth->thw().SetGrbitPlrTrav(lpth->thw().grbitPlrTrav() | grbitPlr);
                                lpthDest->thw().SetGrbitPlrTrav(lpthDest->thw().grbitPlrTrav() | grbitPlr);
                                lpthDest->thw().SetGrbitPlr(lpthDest->thw().grbitPlr() | grbitPlr);
                                lpfl->pt = lpthDest->pt();
                                lpord[1].pt = lpthDest->pt();
                            }
                        }
                        if (lpfl->idPlanet == idPlanetDeepSpace && FFindNearestObject(app, lpfl->pt, static_cast<GrobjClass>(grobjPlanet | mdExact), &scan)) {
                            lpfl->idPlanet = scan.idpl;
                        }
                        lpord->pt = lpfl->pt;
                        lpord->id = lpfl->idPlanet;
                        lpord->grobj = lpfl->idPlanet == idPlanetDeepSpace ? 4 : 1;
                        if (fGotEnufFuel) {
                            wtFuel2Dest = EstFuelUse(app, lpfl, 0, -1, -1, 0);
                            if (wtFuel2Dest > lpfl->rgwtMin[4]) {
                                if (ProductionIO().pfnFleetStat(app, lpfl, 1) > wtFuel2Dest) {
                                    lpfl->rgwtMin[4] = wtFuel2Dest;
                                } else {
                                    lpfl->rgwtMin[4] = ProductionIO().pfnFleetStat(app, lpfl, 1);
                                }
                            }
                        }
                    }
                }
            }
        }
        if (!fDone) {
            if (cPass++ < 10)
                goto MoveUnfinishedFleets;
        }
        KillUsedWaypoints(app);
    }
    return;
}

std::int16_t FTravelThroughMineFields(APPSESSION &app, FLEET *lpfl, std::int16_t *pdTravel, THING *lpthHit) {
    std::int32_t       d2Closest;
    std::int16_t       rgishInc[16];
    std::int16_t       dTravel;
    POINT16       ptAct;
    std::int16_t       iWarp;
    POINT16       ptDst;
    std::int16_t       dy;
    std::int32_t       d2;
    std::int16_t       j;
    std::int16_t       dEnd;
    FLEET         flSrc;
    std::int32_t       dpsh;
    std::int16_t       cshT;
    std::int32_t       dmgReduce;
    std::int32_t       dmgToApply;
    std::int16_t       i;
    THING        *lpth;
    std::int16_t       dmgExtra;
    std::int16_t       cshDamaged;
    std::int16_t       fMineExpert;
    POINT16       ptSrc;
    std::int16_t       iPlayer;
    std::int16_t       cFields;
    std::int16_t       dStart;
    FLEET         flDead;
    THING        *lpthMac;
    std::int32_t       csh;
    std::int16_t       rgi[3];
    std::int16_t       pct;
    std::int32_t       dmgTot;
    std::int16_t       cshDead;
    std::int16_t       rgcField[3];
    RaceAttribute raMajor;
    std::int16_t       dx;
    std::int32_t       dpShield;
    MineFieldType iType;
    std::int16_t       rgFieldE[3][8];
    THING        *lpthClosest;
    std::int16_t       cishInc;
    THING        *lpthSalvage;
    std::int16_t       fHasRamScoop;
    std::int16_t       dmgPer;
    std::int16_t       rgFieldS[3][8];
    std::int16_t       cEngines;
    std::int32_t       dmgPerShip;
    std::uint16_t      ibit;

    lpthSalvage = nullptr;
    cshDead = 0;
    /* A detonation (lpthHit) passes no travel distance. The original read
       through the NULL near pointer anyway, from the start of its data
       segment. */
    dTravel = lpthHit ? 0 : *pdTravel;
    cishInc = 0;
    iPlayer = lpfl->iPlayer;
    raMajor = static_cast<RaceAttribute>(RaceQueryIO().pfnStat(&app.state.rgplr[iPlayer], rsMajorAdv));
    fMineExpert = (raMajor == raMines) * 2 + (raMajor == raStealth);
    if (lpthHit) {
        iWarp = 0;
        goto LHitSkip1;
    }
    ptSrc = lpfl->pt;
    ptDst = lpfl->lpplord->rgord()[1].pt;
    for (iWarp = 3; iWarp < 10 && iWarp * iWarp < dTravel - 1; iWarp++) {
    }
    if (iWarp <= fMineExpert + 3 || (ptSrc.x == ptDst.x && ptSrc.y == ptDst.y)) {
        return 1;
    }
    for (i = 0; i < 3; i++) {
        rgcField[i] = 0;
    }
    lpth = app.state.lpThings;
    lpthMac = app.state.lpThings + app.state.cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->iplr() != iPlayer && lpth->ith() == ithMinefield && app.state.rgplr[lpth->iplr()].rgmdRelation[iPlayer] != 1 &&
            FIntersectCircleLine(ptSrc, ptDst, lpth->pt(), lpth->thm().cMines(), dTravel, &dStart, &dEnd)) {
            iType = static_cast<MineFieldType>(lpth->thm().iType());
            for (i = 0; i < rgcField[iType] && rgFieldE[iType][i] < dStart; i++) {
            }
            if (i == rgcField[iType]) {
                if (i < 8) {
                    rgFieldS[iType][i] = dStart;
                    rgFieldE[iType][i] = dEnd;
                    rgcField[iType]++;
                }
            } else if (dEnd < rgFieldS[iType][i] - 1) {
                if (rgcField[iType] < 8) {
                    for (j = rgcField[iType]; j > i; j--) {
                        rgFieldS[iType][j] = rgFieldS[iType][j - 1];
                        rgFieldE[iType][j] = rgFieldE[iType][j - 1];
                    }
                    rgFieldS[iType][i] = dStart;
                    rgFieldE[iType][i] = dEnd;
                    rgcField[iType]++;
                }
            } else {
                if (dStart < rgFieldS[iType][i]) {
                    rgFieldS[iType][i] = dStart;
                }
                if (dEnd > rgFieldE[iType][i]) {
                    rgFieldE[iType][i] = dEnd;
                    for (j = i + 1; j < rgcField[iType] && rgFieldS[iType][j] <= dEnd; j++) {
                    }
                    if (rgFieldE[iType][j - 1] > dEnd) {
                        rgFieldE[iType][i] = rgFieldE[iType][j - 1];
                    }
                    i++;
                    for (; j < rgcField[iType]; j++) {
                        rgFieldS[iType][i] = rgFieldS[iType][j];
                        rgFieldE[iType][i] = rgFieldE[iType][j];
                        i++;
                    }
                    rgcField[iType] -= j - i;
                }
            }
        }
    }
    cFields = rgcField[0] + rgcField[1] + rgcField[2];
    if (cFields == 0) {
        return 1;
    }
LHitSkip1:
    fHasRamScoop = 0;
    csh = 0;
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            csh += lpfl->rgcsh[i];
            j = app.state.rglpshdef[iPlayer][i].hul.rghs[0].iItem;
            if (LpengineFromId(j)->rgcFuelUsed[4] == 0) {
                fHasRamScoop = 1;
            }
        }
    }
    if (lpthHit) {
        iType = static_cast<MineFieldType>(lpthHit->thm().iType());
        goto LHitSkip2;
    }
    rgi[2] = 0;
    rgi[1] = 0;
    rgi[0] = 0;
    while (cFields > 0) {
        dStart = 10000;
        iType = mineNone;
        for (i = 0; i < 3; i++) {
            if (rgi[i] < rgcField[i] && dStart > rgFieldS[i][rgi[i]]) {
                dStart = rgFieldS[i][rgi[i]];
                iType = static_cast<MineFieldType>(i);
            }
        }
        dEnd = rgFieldE[iType][rgi[iType]] - rgFieldS[iType][rgi[iType]];
        if (iWarp <= rgiWarpSafe[iType] + fMineExpert)
            goto LDoNext;
        pct = (iWarp - rgiWarpSafe[iType] - fMineExpert) * rgpctMineHit[iType];
        for (i = 0; i < dEnd; i++) {
            if (RandomIO().pfnRandom(app, 1000) < pct)
                break;
        }
        if (i == dEnd)
            goto LDoNext;
        dEnd = dStart + i;
    LHitSkip2:
        cshDead = 0;
        dmgTot = 0;
        if (rgrgdmgMine[iType][fHasRamScoop] == 0)
            goto LFinishHit;
        dmgPer = rgrgdmgMine[iType][fHasRamScoop];
        dmgExtra = rgrgdmgMinMine[iType][fHasRamScoop] - rgrgdmgMine[iType][fHasRamScoop] * LOWORD(csh);
        if (csh >= 5 || dmgExtra <= 0) {
            dmgExtra = 0;
        }
        flSrc = *lpfl;
        flDead = {};
        flDead.iPlayer = flSrc.iPlayer;
        flDead.fDead = 1;
        flDead.det = detAll;
        for (i = 0; i < 16; i++) {
            if (lpfl->rgcsh[i] > 0 &&
                (!lpthHit || lpthHit->iplr() != lpfl->iPlayer ||
                 (app.state.rglpshdef[lpfl->iPlayer][i].hul.ihuldef != ihuldefMiniMineLayer && app.state.rglpshdef[lpfl->iPlayer][i].hul.ihuldef != ihuldefSuperMineLayer))) {
                cshT = lpfl->rgcsh[i];
                rgishInc[cishInc++] = i;
                cEngines = app.state.rglpshdef[lpfl->iPlayer][i].hul.rghs[0].cItem;
                dpShield = static_cast<std::uint32_t>((cshT * DpShieldOfShdef(app, app.state.rglpshdef[iPlayer] + i, iPlayer)));
                dpsh = static_cast<std::uint32_t>(app.state.rglpshdef[iPlayer][i].hul.dp);
                dmgToApply = static_cast<std::uint32_t>(((static_cast<std::uint32_t>((cshT * dmgPer)) + dmgExtra) * cEngines));
                dmgTot += dmgToApply;
                dmgReduce = dpShield < static_cast<std::int32_t>((dmgToApply >> 1)) ? dpShield : static_cast<std::int32_t>((dmgToApply >> 1));
                dmgToApply -= dmgReduce;
                cshDamaged = LOWORD(static_cast<std::int32_t>((lpfl->rgcsh[i] * lpfl->rgdv[i].pctSh())) / 100);
                dmgToApply += static_cast<std::int32_t>((static_cast<std::uint32_t>((dpsh * lpfl->rgdv[i].pctDp())) * cshDamaged)) / 500;
                dmgExtra = 0;
                dmgPerShip = static_cast<std::int32_t>((dmgToApply / cshT));
                if (dmgPerShip > dpsh) {
                    cshDead += cshT;
                    flDead.rgcsh[i] = cshT;
                    flSrc.rgcsh[i] = 0;
                    cshT = 0;
                } else {
                    flSrc.rgdv[i].SetPctSh(100);
                    flSrc.rgdv[i].SetPctDp(LOWORD(static_cast<std::int32_t>((static_cast<std::int32_t>((dmgPerShip * 500)) / dpsh))));
                    if (flSrc.rgdv[i].pctDp() == 0) {
                        flSrc.rgdv[i].SetPctDp(1);
                    }
                }
                dmgToApply = 0;
            }
        }
        /* Every ship is immune to this detonation (the owner's mine layers
           in its own field). TRUE leaves ThingDecay free to check the fleet
           against other detonating fields. */
        if (lpthHit && dmgTot == 0) {
            return 1;
        }
        if (cshDead != csh) {
            FleetTransferCargoBalance(app, &flSrc, &flDead);
        }
        *lpfl = flSrc;
        if (cshDead == csh) {
            lpfl->fDead = 1;
        }
    LFinishHit:
        if (!lpthHit) {
            dx = ptDst.x - ptSrc.x;
            dy = ptDst.y - ptSrc.y;
            dTravel = LOWORD(static_cast<std::int32_t>((static_cast<double>(std::sqrt(static_cast<double>((static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * static_cast<std::int16_t>((ptDst.y - ptSrc.y)))))))) + 0.5)));
            ptAct.x = LMulDiv(dx, dEnd, dTravel) + ptSrc.x;
            ptAct.y = LMulDiv(dy, dEnd, dTravel) + ptSrc.y;
            if (cshDead != 0) {
                lpthSalvage = app.state.lpThings;
                lpthMac = app.state.lpThings + app.state.cThing;
                for (; lpthSalvage < lpthMac &&
                       (lpthSalvage->pt().x() != ptAct.x || lpthSalvage->pt().y() != ptAct.y || lpthSalvage->ith() != ithMineralPacket || lpthSalvage->thp().iWarp() != 0);
                     lpthSalvage++) {
                }
                if (lpthSalvage == lpthMac) {
                    lpthSalvage = nullptr;
                }
                DropSalvage(app, &lpthSalvage, lpfl->rgwtMin, flSrc.iplr(), &ptAct);
            }
            d2Closest = 100000000;
            lpth = app.state.lpThings;
            lpthMac = app.state.lpThings + app.state.cThing;
            for (; lpth < lpthMac; lpth++) {
                if (lpth->iplr() != iPlayer && lpth->ith() == ithMinefield && app.state.rgplr[lpth->iplr()].rgmdRelation[iPlayer] != 1 && lpth->thm().iType() == iType) {
                    dx = lpth->pt().x() - ptAct.x;
                    dy = lpth->pt().y() - ptAct.y;
                    d2 = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * static_cast<std::int16_t>((lpth->pt().y() - ptAct.y)))) - lpth->thm().cMines();
                    if (d2 < d2Closest) {
                        d2Closest = d2;
                        lpthClosest = lpth;
                    }
                }
            }
            d2 = static_cast<std::int32_t>((lpthClosest->thm().cMines() / 20));
            if (d2 > 50) {
                d2 = static_cast<std::int32_t>((lpthClosest->thm().cMines() / 100));
                if (d2 < 50) {
                    d2 = 50;
                }
            } else if (d2 < 10) {
                d2 = 10;
            }
        } else {
            ptAct = lpthHit->pt();
            lpthClosest = lpthHit;
        }
        if (RaceQueryIO().pfnStat(&app.state.rgplr[lpthClosest->iplr()], rsMajorAdv) == raMines) {
            ibit = 1 << lpthClosest->iplr();
            if (cishInc == 0) {
                for (i = 0; i < 16; i++) {
                    if (lpfl->rgcsh[i] > 0) {
                        app.state.rglpshdef[iPlayer][i].grbitPlr |= ibit;
                    }
                }
            } else {
                for (i = 0; i < cishInc; i++) {
                    app.state.rglpshdef[iPlayer][rgishInc[i]].grbitPlr = app.state.rglpshdef[iPlayer][rgishInc[i]].grbitPlr | ibit;
                }
            }
        }
        if (dmgTot > 32760) {
            dmgTot = 32760;
        }
        if (dmgTot == 0) {
            if (iPlayer != lpthClosest->iplr()) {
                TurnIO().pfnMessage(app, iPlayer, idmHasStoppedMineField, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id | 0x8000, lpthClosest->iplr(), iType, ptAct.x, ptAct.y, 0, 0);
            }
            TurnIO().pfnMessage(app, lpthClosest->iplr(), idmHasStoppedMineField2, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id | 0x8000, iType, ptAct.x, ptAct.y, 0, 0, 0);
        } else if (cshDead == 0) {
            if (iPlayer != lpthClosest->iplr()) {
                TurnIO().pfnMessage(app, iPlayer, !lpthHit ? idmHasStoppedMineFieldFleetHasTaken : idmHasDamagedDetonatingMineFieldFleetHas, static_cast<MsgGoto>(lpfl->id | 0x8000),
                            lpfl->id | 0x8000, lpthClosest->iplr(), iType, ptAct.x, ptAct.y, LOWORD(dmgTot), 0);
            }
            TurnIO().pfnMessage(app, lpthClosest->iplr(), !lpthHit ? idmHasStoppedMineFieldMinesHaveInflicted : idmHasDamagedDetonatingMineFieldMinesHave, static_cast<MsgGoto>(lpfl->id | 0x8000),
                        lpfl->id | 0x8000, iType, ptAct.x, ptAct.y, LOWORD(dmgTot), 0, 0);
        } else if (cshDead < csh) {
            if (iPlayer != lpthClosest->iplr()) {
                TurnIO().pfnMessage(app, iPlayer, !lpthHit ? idmHasStoppedMineFieldFleetHasTaken2 : idmHasTakenDamageDetonatingMineFieldFleet, static_cast<MsgGoto>(lpfl->id | 0x8000),
                            lpfl->id | 0x8000, lpthClosest->iplr(), iType, ptAct.x, ptAct.y, LOWORD(dmgTot), cshDead);
            }
            TurnIO().pfnMessage(app, lpthClosest->iplr(), !lpthHit ? idmHasStoppedMineFieldMinesHaveInflicted2 : idmHasDamagedDetonatingMineFieldMinesHave2, static_cast<MsgGoto>(lpfl->id | 0x8000),
                        lpfl->id | 0x8000, iType, ptAct.x, ptAct.y, LOWORD(dmgTot), cshDead, 0);
        } else if (lpthSalvage) {
            flDead.id = lpfl->id;
            if (iPlayer != lpthClosest->iplr()) {
                TurnIO().pfnMessage(app, iPlayer, idmHasAnnihilatedMineField, gotoThing, lpthSalvage->idFull(), WFromLpfl(app, &flDead), lpthClosest->iplr(), iType, ptAct.x, ptAct.y,
                            0);
            }
            TurnIO().pfnMessage(app, lpthClosest->iplr(), idmHasAnnihilatedMineField2, gotoThing, lpthSalvage->idFull(), lpfl->id, iType, ptAct.x, ptAct.y, 0, 0);
        } else {
            flDead.id = lpfl->id;
            if (iPlayer != lpthClosest->iplr()) {
                TurnIO().pfnMessage(app, iPlayer, idmHasAnnihilatedMineField3, gotoNone, WFromLpfl(app, &flDead), lpthClosest->iplr(), iType, ptAct.x, ptAct.y, 0, 0);
            }
            if (lpfl->iPlayer == lpthClosest->iplr()) {
                TurnIO().pfnMessage(app, lpthClosest->iplr(), idmHasAnnihilatedMineField4, gotoNone, WFromLpfl(app, &flDead), iType, ptAct.x, ptAct.y, 0, 0, 0);
            } else {
                TurnIO().pfnMessage(app, lpthClosest->iplr(), idmHasAnnihilatedMineField2, gotoThing, lpthClosest->idFull(), lpfl->id, iType, ptAct.x, ptAct.y, 0, 0);
            }
        }
        if (!lpthHit) {
            if (d2 >= lpthClosest->thm().cMines()) {
                FreeLpth(app, lpthClosest);
            } else {
                lpthClosest->thm().SetCMines(lpthClosest->thm().cMines() - d2);
                lpthClosest->thm().SetGrbitPlrNow(lpthClosest->thm().grbitPlrNow() | (1 << iPlayer));
                lpthClosest->thm().SetGrbitPlr(lpthClosest->thm().grbitPlr() | (1 << iPlayer));
            }
            *pdTravel = dEnd;
        }
        lpfl->fNoHeal = 1;
        return 0;
    LDoNext:
        rgi[iType]++;
        cFields--;
    }
    return 1;
}
