#include "common.h"
#include "appstate.h"
#include "text_io.h"
#include "colony_io.h"
#include "planet_query_io.h"
#include "research_io.h"
#include "part_query_io.h"
#include "alert_io.h"
#include "capacity_io.h"
#include "record_io.h"

std::uint16_t mpicolgrbitBU[12] = {255, 255, 255, 255, 255, 255, 255, 8, 16, 32, 64, 128};

const char *PszGetDestName(APPSESSION &app, FLEET *lpfl, HDC hdc) {
    std::int16_t i;
    ORDER   ord;

    ord = lpfl->lpplord->rgord()[0];
    if (lpfl->cord <= 1)
        goto LNoDest;

    if (ord.fValidTask) {
        switch (ord.grTask) {
        case grTaskXfer:
            for (i = 0; i < 5; i++) {
                if (ord.txp.rgia[i].iAction() == iActionWaitPercent)
                    goto LDelayed;
            }
            break;
        case grTaskColonize:
            if (ord.grobj != grobjPlanet)
                break;
            goto LNoDest;
        case grTaskMerge:
        case grTaskScrap:
            goto LNoDest;
        case grTaskLayMines:
        LDelayed:
            if (hdc) {
                SetHdcTextColor(hdc, 127);
            }
            return TextIO().pfnCompressed(app, idsDelayed);
        default:
            break;
        }
    }
    ord = lpfl->lpplord->rgord()[1];
    return PszGetLocName(app, static_cast<GrobjClass>(ord.grobj), ord.id, ord.pt.x, ord.pt.y);

LNoDest:
    return app.state.szDblDash;
}

std::int16_t FDestIsWP0(FLEET *lpfl) {
    std::int16_t i;
    ORDER   ord;

    ord = lpfl->lpplord->rgord()[0];
    if (lpfl->cord > 1) {
        if (ord.fValidTask) {
            switch (ord.grTask) {
            case grTaskXfer:
                for (i = 0; i < 5; i++) {
                    if (ord.txp.rgia[i].iAction() == iActionWaitPercent) {
                        return 1;
                    }
                }
                return 0;
            case grTaskColonize:
                if (ord.grobj != grobjPlanet)
                    break;
                /* fallthrough */
            case grTaskMerge:
            case grTaskScrap:
            case grTaskLayMines:
                return 1;
            }
        }
        return 0;
    }
    return 1;
}

const char *PszGetETA(APPSESSION &app, HDC hdc, FLEET *lpfl, std::int16_t *pcYears) {
    POINT16 pt;
    std::int16_t c;
    std::int16_t i;
    ORDER   ord;
    char   *psz;

    ord = lpfl->lpplord->rgord()[0];
    pt = ord.pt;
    if (lpfl->cord > 1) {
        if (ord.fValidTask) {
            switch (ord.grTask) {
            case grTaskXfer:
                for (i = 0; i < 5; i++) {
                    if (ord.txp.rgia[i].iAction() == iActionWaitPercent)
                        goto LNoETA;
                }
                break;
            case grTaskColonize:
                if (ord.grobj == grobjPlanet)
                    goto LNoETA;
            default:
                break;
            case grTaskMerge:
            case grTaskScrap:
            case grTaskLayMines:
                goto LNoETA;
            }
        }
        ord = lpfl->lpplord->rgord()[1];
        CchGetETA(app, hdc, lpfl, app.state.szWork, 1, 1);
        if (hdc && EstFuelUse(app, lpfl, 0, ord.iWarp, -1, 0) > lpfl->rgwtMin[4]) {
            SetHdcTextColor(hdc, 0xff);
        }
        if (pcYears) {
            psz = app.state.szWork;
            c = 0;
            for (; *psz >= '0' && *psz <= '9'; psz++) {
                c = 10 * c + (*psz - '0');
            }
            if (c == 0) {
                c = 32000;
            }
            *pcYears = c;
        }
        return app.state.szWork;
    }
LNoETA:
    if (pcYears) {
        *pcYears = 0;
    }
    return app.state.szDblDash;
}

const char *PszGetTaskName(APPSESSION &app, FLEET *lpfl, std::int16_t *picr) {
    std::int16_t        icr;
    StringId       ids;
    XferActionType opOrd;
    std::int16_t        iZip;
    std::int16_t        i;
    ORDER          ord;
    std::int16_t        fPercent;
    char          *psz;

    icr = -1;
    ord = lpfl->lpplord->rgord()[0];
    *picr = -1;
    if (ord.fValidTask) {
        switch (ord.grTask) {
        case grTaskXfer:
            for (i = 0; i < 4; i++) {
                if (ord.txp.rgia[i].iAction() == iActionWaitPercent)
                    goto LShowTask;
            }
            break;
        case grTaskColonize:
        case grTaskMerge:
        case grTaskScrap:
        case grTaskLayMines:
        case grTaskAutoRoute:
            goto LShowTask;
        }
    }
    if (lpfl->cord > 1) {
        ord = lpfl->lpplord->rgord()[1];
    }
LShowTask:
    if (ord.fValidTask) {
        ids = static_cast<StringId>(idsTaskHere + ord.grTask);
        switch (ord.grTask) {
        case grTaskXfer:
            for (i = 0; i < 4; i++) {
                if (app.state.vrgZip[i].fValid && std::memcmp(&app.state.vrgZip[i], &ord.txp, 10) == 0)
                    return app.state.vrgZip[i].szName;
            }
            ids = idsAction;
            iZip = -1;
            if (ord.txp.rgia[4].iAction() == iActionLoadDunnage &&
                (ord.txp.rgia[3].iAction() == iActionNone || (ord.txp.rgia[3].iAction() == iActionUnloadAll && ord.txp.rgia[0].iAction() == iActionUnloadAll))) {
                opOrd = static_cast<XferActionType>(ord.txp.rgia[0].iAction());
                switch (opOrd) {
                default:
                    break;
                case iActionLoadAll:
                case iActionUnloadAll:
                case iActionWaitPercent:
                    for (i = 1; i < 3 && ord.txp.rgia[i].iAction() == opOrd; i++) {
                    }
                    if (i == 3) {
                        switch (opOrd) {
                        default:
                            break;
                        case iActionLoadAll:
                            iZip = 0;
                            break;
                        case iActionUnloadAll:
                            iZip = 1;
                            break;
                        case iActionWaitPercent:
                            iZip = 2;
                        }
                        return app.state.rgszZipOrder[iZip];
                    }
                }
            }
            for (i = 4; i >= 0; i--) {
                opOrd = static_cast<XferActionType>(ord.txp.rgia[i].iAction());
                if (idsAction + static_cast<int>(opOrd) > static_cast<std::int16_t>(ids)) {
                    ids = static_cast<StringId>(idsAction + static_cast<int>(opOrd));
                    icr = i;
                }
            }
            if (ids == idsAction) {
                return TextIO().pfnCompressed(app, idsTransport);
            }
            opOrd = static_cast<XferActionType>(ord.txp.rgia[icr].iAction());
            *picr = icr;
            fPercent = 0;
            switch (opOrd) {
            case iActionLoadDunnage:
                if (icr == 4) {
                    ids = idsLoadOptimal;
                }
                /* fallthrough */
            case iActionLoadAll:
            case iActionUnloadAll:
                return TextIO().pfnCompressed(app, ids);
            case iActionFillPercent:
            case iActionWaitPercent:
                fPercent = 1;
                /* fallthrough */
            case iActionUnloadExact:
            case iActionSetAmount:
            case iActionSetWaypoint:
                psz = TextIO().pfnCompressed(app, ids);
                psz[std::strlen(psz) - 3] = 0;
                if (fPercent) {
                    CchSprintf(app.state.szWork, "%s %d%%", psz, ord.txp.rgia[icr].cQuan());
                } else {
                    CchSprintf(app.state.szWork, icr == 4 ? "%s %dmg" : "%s %dkT", psz, ord.txp.rgia[icr].cQuan());
                }
                return app.state.szWork;
            default:
                return app.state.szDblDash;
            }
        case grTaskLayMines:
            if (ord.tlm().cTime < 5) {
                CchSprintf(app.state.szWork, "%s  %dy", TextIO().pfnCompressed(app, ids), ord.tlm().cTime + 1);
            } else {
                TextIO().pfnCchGetString(app, ids, app.state.szWork);
            }
            return app.state.szWork;
        case grTaskPatrol:
            if (ord.tptl().iDist < 11) {
                CchSprintf(app.state.szWork, "%s  %dly", TextIO().pfnCompressed(app, ids), (ord.tptl().iDist + 1) * 50);
            } else {
                TextIO().pfnCchGetString(app, ids, app.state.szWork);
            }
            return app.state.szWork;
        case grTaskAutoRoute:
        default:
            return TextIO().pfnCompressed(app, ids);
        }
    }
    return app.state.szDblDash;
}

void DumpUniverse(APPSESSION &app) {
    StringId ids;
    std::int16_t  i;
    std::int16_t  fOpen;
    std::int16_t  fSuccess;
    std::int16_t  fSilentSav;
    std::int16_t  cch;

    fSilentSav = app.state.fFileErrSilent;
    fSuccess = 1;
    fOpen = 0;
    if (app.state.game.lid == 0 || app.state.idPlayer == iplrNone) {
        fSuccess = 0;
        goto DisplayStatus;
    } else {
        StarsRecoveryScope scopeRecovery;
        try {
            app.state.fFileErrSilent = 1;
            CchSprintf(app.state.szWork, "%s.map", app.state.szBase);
            RecordIO().pfnStreamOpen(app, app.state.szWork, mdCreate);
            fOpen = 1;
            RgToStream(app, const_cast<void *>(reinterpret_cast<const void*>("#\tX\tY\tName\r\n")), 12);
            for (i = 0; i < app.state.game.cPlanMax; i++) {
                cch = CchSprintf(app.state.szWork, "%d\t%d\t%d\t%s\r\n", i + 1, app.state.rgptPlan[i].x, app.state.rgptPlan[i].y, PszGetCompressedPlanet(app, app.state.rgidPlan[i]));
                RgToStream(app, app.state.szWork, cch);
            }
            RecordIO().pfnStreamClose(app);
        } catch (const StarsError &) {
            if (fOpen) {
                RecordIO().pfnStreamClose(app);
            }
            app.state.fFileErrSilent = fSilentSav;
            fSuccess = 0;
            goto DisplayStatus;
        }
    }
DisplayStatus:
    ids = !fSuccess ? idsUnableWriteUniverseDefinitionSMapOperation : idsUniverseDefinitionHasSuccessfullyWrittenSMap;
    CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, ids), app.state.szBase);
    if (fSuccess) {
        AlertIO().pfnAlertSz(app, app.state.szWork, win32::MB_ICONASTERISK);
    } else {
        AlertIO().pfnAlertSz(app, app.state.szWork, win32::MB_ICONHAND);
    }
    app.state.fFileErrSilent = fSilentSav;
    return;
}

void DumpPlanets(APPSESSION &app) {
    PLANET  *lpplMac;
    StringId ids;
    PLANET  *lppl;
    char     szFile[256];
    char     szForm[256];
    std::int16_t  j;
    std::int16_t  i;
    std::int16_t  fOpen;
    std::int16_t  fSuccess;
    std::int16_t  fSilentSav;
    char    *psz;
    std::int16_t  cch;
    std::int32_t  l;
    float    pct;
    std::int32_t  rgl[4];
    PART     part;

    fSilentSav = app.state.fFileErrSilent;
    fSuccess = 1;
    fOpen = 0;
    if (app.state.game.lid == 0 || app.state.idPlayer == iplrNone) {
        fSuccess = 0;
        goto DisplayStatus;
    } else {
        StarsRecoveryScope scopeRecovery;
        try {
            app.state.fFileErrSilent = 1;
            if (app.state.gd.fPerPlayerDumps) {
                CchSprintf(szFile, "%s.p%d", app.state.szBase, app.state.idPlayer + 1);
            } else {
                CchSprintf(szFile, "%s.pla", app.state.szBase);
            }
            RecordIO().pfnStreamOpen(app, szFile, mdCreate);
            fOpen = 1;
            j = app.state.gd.fPerPlayerDumps + 2;
            for (i = 0; i < j; i++) {
                cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsPlanetNameOwnerStarbaseTypeReportAge + i), szForm);
                for (psz = szForm; *psz != 0; psz++) {
                    if (*psz == '*') {
                        *psz = '\t';
                    }
                }
                RgToStream(app, szForm, cch);
                if (i == j - 1) {
                    RgToStream(app, app.state.szCRLF, 2);
                }
            }
            lppl = app.state.lpPlanets;
            lpplMac = app.state.lpPlanets + app.state.cPlanet;
            for (; lppl < lpplMac; lppl++) {
                std::strcpy(szForm, PszGetCompressedPlanet(app, app.state.rgidPlan[lppl->id]));
                RgToStream(app, szForm, std::strlen(szForm));
                szForm[0] = '\t';
                if (lppl->iPlayer == iplrNone) {
                    RgToStream(app, szForm, 1);
                } else {
                    std::strcpy(&szForm[1], TextIO().pfnPlayerName(app, lppl->iPlayer, 1, 0, 0, 0, nullptr));
                    RgToStream(app, szForm, std::strlen(szForm));
                }
                if (lppl->iPlayer == iplrNone || !lppl->fStarbase) {
                    cch = 1;
                } else {
                    std::strcpy(&szForm[1], app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].hul.szClass);
                    cch = std::strlen(szForm);
                }
                RgToStream(app, szForm, cch);
                CchSprintf(std::span{szForm}.subspan(1), "%d", app.state.game.turn - lppl->turn);
                RgToStream(app, szForm, std::strlen(szForm));
                szForm[1] = 0;
                if (lppl->det == detAll) {
                    std::strcpy(&szForm[1], PszFromLong(app, static_cast<std::uint32_t>((lppl->rgwtMin[3] * 100)), nullptr));
                } else if (lppl->iPlayer != iplrNone && lppl->det >= detSome) {
                    l = static_cast<std::uint32_t>((lppl->uPopGuess * 400));
                    std::strcpy(&szForm[1], PszFromLong(app, l, nullptr));
                }
                RgToStream(app, szForm, std::strlen(szForm));
                if (lppl->det < detSome) {
                    szForm[1] = 0;
                } else {
                    i = PlanetQueryIO().pfnDesirability(app, lppl, app.state.idPlayer);
                    CchSprintf(std::span{szForm}.subspan(1), app.state.PCTDPCTPCT, i);
                }
                RgToStream(app, szForm, std::strlen(szForm));
                szForm[1] = 0;
                if (lppl->det == detAll) {
                    PszProdQueueTop(app, lppl, nullptr);
                    std::strcpy(&szForm[1], app.state.szWork);
                }
                RgToStream(app, szForm, std::strlen(szForm));
                if (lppl->det == detAll) {
                    CalcPctSurvive(app, lppl, &pct, nullptr);
                    pct = static_cast<float>((static_cast<double>(1.0) - pct));
                    CchSprintf(std::span{szForm}.subspan(1), "%ld\t%ld\t%d.%d%%", static_cast<std::int32_t>(lppl->cMines), static_cast<std::int32_t>(lppl->cFactories), LOWORD(static_cast<std::int32_t>((static_cast<double>(pct) * 100))),
                               LOWORD(static_cast<std::int32_t>(((pct - static_cast<double>(static_cast<std::int16_t>(LOWORD(static_cast<std::int32_t>((static_cast<double>(pct) * 100))))) / 100.0) * 10000))));
                } else {
                    szForm[2] = '\t';
                    szForm[1] = '\t';
                    szForm[3] = 0;
                    if (app.state.gd.fPerPlayerDumps && lppl->uDefGuess != 0) {
                        cch = CchSprintf(std::span{szForm}.subspan(3), "%d%%", lppl->uDefGuess * 6 + 3);
                        szForm[cch + 3] = 0;
                    }
                }
                RgToStream(app, szForm, std::strlen(szForm));
                szForm[1] = 0;
                for (i = 0; i < 3; i++) {
                    if (lppl->det >= detSome) {
                        std::strcpy(&szForm[1], PszFromLong(app, lppl->rgwtMin[i], nullptr));
                    }
                    RgToStream(app, szForm, std::strlen(szForm));
                }
                szForm[1] = 0;
                for (i = 0; i < 3; i++) {
                    if (lppl->det >= detMore) {
                        ColonyIO().pfnEstimateMining(app, lppl, rgl, -1, 0);
                        std::strcpy(&szForm[1], PszFromLong(app, rgl[i], nullptr));
                    }
                    RgToStream(app, szForm, std::strlen(szForm));
                }
                szForm[1] = 0;
                for (i = 0; i < 3; i++) {
                    if (lppl->det >= detSome) {
                        std::strcpy(&szForm[1], PszFromInt(app, lppl->rgMinConc[i], nullptr));
                    }
                    RgToStream(app, szForm, std::strlen(szForm));
                }
                if (lppl->det == detAll) {
                    std::strcpy(&szForm[1], PszFromInt(app, ResearchIO().pfnResources(app, lppl, app.state.idPlayer), nullptr));
                } else {
                    szForm[1] = 0;
                }
                RgToStream(app, szForm, std::strlen(szForm));
                if (app.state.gd.fPerPlayerDumps) {
                    if (lppl->det >= detSome) {
                        for (i = 0; i < 3; i++) {
                            std::strcpy(&szForm[1], PszCalcEnvVar(app, static_cast<EnvType>(i), lppl->rgEnvVar[i]));
                            RgToStream(app, szForm, std::strlen(szForm));
                        }
                        for (i = 0; i < 3; i++) {
                            std::strcpy(&szForm[1], PszCalcEnvVar(app, static_cast<EnvType>(i), lppl->rgEnvVarOrig[i]));
                            RgToStream(app, szForm, std::strlen(szForm));
                        }
                        std::strcpy(&szForm[1], PszFromInt(app, PctPlanetOptValue(app, lppl, app.state.idPlayer), nullptr));
                        std::strcat(&szForm[1], "%");
                        RgToStream(app, szForm, std::strlen(szForm));
                    } else {
                        szForm[1] = 0;
                        for (i = 0; i < 7; i++) {
                            RgToStream(app, szForm, 1);
                        }
                    }
                    if (lppl->det == detAll) {
                        std::strcpy(&szForm[1], PszFromInt(app, CapacityIO().pfnPctPlanetCapacity(app, lppl), nullptr));
                        RgToStream(app, szForm, std::strlen(szForm));
                        std::strcpy(&szForm[1], PszFromInt(app, GetPlanetScannerRange(app, lppl, &i), nullptr));
                        RgToStream(app, szForm, std::strlen(szForm));
                        std::strcpy(&szForm[1], PszFromInt(app, i, nullptr));
                        RgToStream(app, szForm, std::strlen(szForm));
                        if (lppl->idFling == 0) {
                            i = 0;
                            szForm[1] = 0;
                        } else {
                            i = lppl->iWarpFling + 4;
                            std::strcpy(&szForm[1], TextIO().pfnPlanetName(app, lppl->idFling - 1));
                        }
                        RgToStream(app, szForm, std::strlen(szForm));
                        std::strcpy(&szForm[1], PszFromInt(app, i, nullptr));
                        RgToStream(app, szForm, std::strlen(szForm));
                        if (lppl->idRoute == 0) {
                            szForm[1] = 0;
                        } else {
                            std::strcpy(&szForm[1], TextIO().pfnPlanetName(app, lppl->idRoute - 1));
                        }
                        RgToStream(app, szForm, std::strlen(szForm));
                        if (lppl->fStarbase) {
                            i = IStargateFromLppl(app, lppl);
                            if (i != -1) {
                                part.hs.grhst = hstSpecialSB;
                                part.hs.iItem = i;
                                PartQueryIO().pfnLookup(app, &part);
                                std::strcpy(&szForm[1], PszFromInt(app, part.pspecialsb()->grAbility2, nullptr));
                                RgToStream(app, szForm, std::strlen(szForm));
                                std::strcpy(&szForm[1], PszFromInt(app, part.pspecialsb()->grAbility, nullptr));
                                RgToStream(app, szForm, std::strlen(szForm));
                            }
                        } else {
                            i = -1;
                        }
                        if (i == -1) {
                            RgToStream(app, const_cast<void *>(reinterpret_cast<const void*>("\t0\t0")), 4);
                        }
                        if (lppl->fStarbase) {
                            i = lppl->pctDp;
                        } else {
                            i = 0;
                        }
                        std::strcpy(&szForm[1], PszFromInt(app, i, nullptr));
                        RgToStream(app, szForm, std::strlen(szForm));
                    }
                }
                RgToStream(app, app.state.szCRLF, 2);
            }
            RecordIO().pfnStreamClose(app);
        } catch (const StarsError &) {
            if (fOpen) {
                RecordIO().pfnStreamClose(app);
            }
            app.state.fFileErrSilent = fSilentSav;
            fSuccess = 0;
            goto DisplayStatus;
        }
    }
DisplayStatus:
    ids = !fSuccess ? idsUnableWritePlanetInformationSOperationTerminated : idsKnownPlanetInformationHasSuccessfullyWrittenS;
    CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, ids), szFile);
    if (fSuccess) {
        AlertIO().pfnAlertSz(app, app.state.szWork, win32::MB_ICONASTERISK);
    } else {
        AlertIO().pfnAlertSz(app, app.state.szWork, win32::MB_ICONHAND);
    }
    app.state.fFileErrSilent = fSilentSav;
    return;
}

void DumpFleets(APPSESSION &app) {
    std::int16_t  iplr;
    StringId ids;
    char     szFile[256];
    char     szForm[256];
    std::int16_t  ifl;
    FLEET   *lpfl;
    std::int16_t  j;
    std::int16_t  i;
    std::int16_t  fOpen;
    std::int16_t  fSuccess;
    std::int16_t  fSilentSav;
    char    *psz;
    std::int16_t  cch;
    std::int32_t  l;

    fSilentSav = app.state.fFileErrSilent;
    fSuccess = 1;
    fOpen = 0;
    iplr = app.state.idPlayer;
    if (app.state.game.lid == 0 || app.state.idPlayer == iplrNone) {
        fSuccess = 0;
        goto DisplayStatus;
    } else {
        StarsRecoveryScope scopeRecovery;
        try {
            app.state.fFileErrSilent = 1;
            if (app.state.gd.fPerPlayerDumps) {
                CchSprintf(szFile, "%s.f%d", app.state.szBase, app.state.idPlayer + 1);
            } else {
                CchSprintf(szFile, "%s.fle", app.state.szBase);
            }
            RecordIO().pfnStreamOpen(app, szFile, mdCreate);
            fOpen = 1;
            j = app.state.gd.fPerPlayerDumps + 2;
            for (i = 0; i < j; i++) {
                cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsFleetNameXYPlanetDestinationBattle + i), szForm);
                for (psz = szForm; *psz != 0; psz++) {
                    if (*psz == '*') {
                        *psz = '\t';
                    }
                }
                RgToStream(app, szForm, cch);
                if (i == j - 1) {
                    RgToStream(app, app.state.szCRLF, 2);
                }
            }
            iplr = app.state.idPlayer;
            for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                lpfl = app.state.rglpfl[ifl];
                if (!app.state.rglpfl[ifl])
                    break;
                app.state.idPlayer = iplrNone;
                psz = PszGetFleetName(app, lpfl->id);
                app.state.idPlayer = iplr;
                RgToStream(app, psz, std::strlen(psz));
                szForm[0] = '\t';
                std::strcpy(&szForm[1], PszFromInt(app, lpfl->pt.x, nullptr));
                RgToStream(app, szForm, std::strlen(szForm));
                std::strcpy(&szForm[1], PszFromInt(app, lpfl->pt.y, nullptr));
                RgToStream(app, szForm, std::strlen(szForm));
                if (lpfl->idPlanet == idPlanetDeepSpace) {
                    cch = 1;
                } else {
                    psz = TextIO().pfnPlanetName(app, lpfl->idPlanet);
                    std::strcpy(&szForm[1], psz);
                    cch = std::strlen(psz) + 1;
                }
                RgToStream(app, szForm, cch);
                if (lpfl->det == detAll) {
                    std::strcpy(&szForm[1], PszGetDestName(app, lpfl, nullptr));
                } else if (app.state.gd.fPerPlayerDumps && lpfl->det < detAll && lpfl->fdirValid()) {
                    std::strcpy(&szForm[1], PszFromInt(app, lpfl->dirFltX(), nullptr));
                    std::strcat(szForm, ".");
                    std::strcat(szForm, PszFromInt(app, lpfl->dirFltY(), nullptr));
                } else {
                    szForm[1] = 0;
                }
                RgToStream(app, szForm, std::strlen(szForm));
                if (app.state.rglpbtlplan[lpfl->iplr()]) {
                    std::strcpy(&szForm[1], app.state.rglpbtlplan[lpfl->iplr()][lpfl->iplan].szName);
                } else {
                    szForm[1] = 0;
                }
                RgToStream(app, szForm, std::strlen(szForm));
                l = 0;
                for (i = 0; i < 16; i++) {
                    l += lpfl->rgcsh[i];
                }
                std::strcpy(&szForm[1], PszFromLong(app, l, nullptr));
                RgToStream(app, szForm, std::strlen(szForm));
                for (i = 0; i < 5; i++) {
                    std::strcpy(&szForm[1], PszFromLong(app, lpfl->rgwtMin[i], nullptr));
                    RgToStream(app, szForm, std::strlen(szForm));
                }
                if (app.state.gd.fPerPlayerDumps) {
                    std::strcpy(&szForm[1], PszFromInt(app, lpfl->iPlayer + 1, nullptr));
                    RgToStream(app, szForm, std::strlen(szForm));
                    if (lpfl->cord > 1) {
                        std::strcpy(&szForm[1], PszGetETA(app, nullptr, lpfl, nullptr));
                    } else {
                        szForm[1] = '0';
                        szForm[2] = 0;
                    }
                    RgToStream(app, szForm, std::strlen(szForm));
                    if (lpfl->cord >= 2) {
                        i = lpfl->lpplord->rgord()[1].iWarp;
                    } else if (lpfl->det < detAll && lpfl->fdirValid()) {
                        i = lpfl->iwarpFlt();
                    } else {
                        i = 0;
                    }
                    std::strcpy(&szForm[1], PszFromInt(app, i, nullptr));
                    RgToStream(app, szForm, std::strlen(szForm));
                    std::strcpy(&szForm[1], PszFromLong(app, WtFromLpfl(app, lpfl), nullptr));
                    RgToStream(app, szForm, std::strlen(szForm));
                    std::strcpy(&szForm[1], PszFromInt(app, PctCloakFromLpfl(app, lpfl), nullptr));
                    RgToStream(app, szForm, std::strlen(szForm));
                    j = GetFleetScannerRange(app, lpfl, &i, nullptr, nullptr);
                    if (j == -1) {
                        j = 0;
                    }
                    std::strcpy(&szForm[1], PszFromInt(app, j, nullptr));
                    RgToStream(app, szForm, std::strlen(szForm));
                    std::strcpy(&szForm[1], PszFromInt(app, i, nullptr));
                    RgToStream(app, szForm, std::strlen(szForm));
                    if (lpfl->det == detAll) {
                        std::strcpy(&szForm[1], PszGetTaskName(app, lpfl, &i));
                    } else {
                        szForm[1] = 0;
                    }
                    RgToStream(app, szForm, std::strlen(szForm));
                    std::strcpy(&szForm[1], PszFromLong(app, ColonyIO().pfnFleetMining(app, lpfl), nullptr));
                    RgToStream(app, szForm, std::strlen(szForm));
                    std::strcpy(&szForm[1], PszFromLong(app, CMineSweepFromLpfl(app, lpfl), nullptr));
                    RgToStream(app, szForm, std::strlen(szForm));
                    std::strcpy(&szForm[1], PszFromLong(app, CLayMinesFromLpfl(app, lpfl, mineAll, ishdefAll), nullptr));
                    RgToStream(app, szForm, std::strlen(szForm));
                    std::strcpy(&szForm[1], PszFromLong(app, PctTerraFromLpfl(app, lpfl), nullptr));
                    RgToStream(app, szForm, std::strlen(szForm));
                    l = 0;
                    for (i = 0; i < 16; i++) {
                        if (lpfl->rgcsh[i] != 0) {
                            j = PartQueryIO().pfnHull(app.state.rglpshdef[lpfl->iPlayer][i].hul.ihuldef)->imdCategory;
                            if (j <= 1 || j >= 6) {
                                l += lpfl->rgcsh[i];
                            }
                        }
                    }
                    std::strcpy(&szForm[1], PszFromLong(app, l, nullptr));
                    RgToStream(app, szForm, std::strlen(szForm));
                    for (j = 2; j < 6; j++) {
                        l = 0;
                        for (i = 0; i < 16; i++) {
                            if (lpfl->rgcsh[i] != 0 && PartQueryIO().pfnHull(app.state.rglpshdef[lpfl->iPlayer][i].hul.ihuldef)->imdCategory == j) {
                                l += lpfl->rgcsh[i];
                            }
                        }
                        std::strcpy(&szForm[1], PszFromLong(app, l, nullptr));
                        RgToStream(app, szForm, std::strlen(szForm));
                    }
                }
                RgToStream(app, app.state.szCRLF, 2);
            }
            RecordIO().pfnStreamClose(app);
        } catch (const StarsError &) {
            if (fOpen) {
                RecordIO().pfnStreamClose(app);
            }
            app.state.fFileErrSilent = fSilentSav;
            fSuccess = 0;
            goto DisplayStatus;
        }
    }
DisplayStatus:
    ids = !fSuccess ? idsUnableWriteFleetInformationSOperationTerminated : idsKnownFleetInformationHasSuccessfullyWrittenS;
    CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, ids), szFile);
    if (fSuccess) {
        AlertIO().pfnAlertSz(app, app.state.szWork, win32::MB_ICONASTERISK);
    } else {
        AlertIO().pfnAlertSz(app, app.state.szWork, win32::MB_ICONHAND);
    }
    app.state.fFileErrSilent = fSilentSav;
    return;
}

// FDumpCmdLineGame writes the dumps a command line asks for (-dm, -dp, -df)
// from the player turn file it names (szBase), as the Windows game does
// when it opens the file (FOpenGame): universe, then planets, then fleets.
// It returns FALSE if the file isn't a player's turn file it can load.
std::int16_t FDumpCmdLineGame(APPSESSION &app) {
    char  szFile[256];
    char *pchDot;
    char *pchDir;

    std::strcpy(szFile, app.state.szBase);
    pchDot = std::strrchr(szFile, '.');
    pchDir = std::strrchr(szFile, chDirSep);
    if (!pchDot || (pchDir && pchDir > pchDot)) {
        return 0;
    }
    *pchDot = 0;
    DestroyCurGame(app);
    std::strcpy(app.state.szBase, szFile);
    app.state.fFileErrSilent = 1;
    if (!FLoadGame(app, szFile, pchDot + 1)) {
        return 0;
    }
    app.state.fFileErrSilent = 0;
    if (app.state.game.lid == 0 || app.state.idPlayer == iplrNone) {
        return 0;
    }
    if (app.state.ini.fDumpMap) {
        DumpUniverse(app);
    }
    if (app.state.ini.fDumpPlanets) {
        DumpPlanets(app);
    }
    if (app.state.ini.fDumpFleets) {
        DumpFleets(app);
    }
    return 1;
}
