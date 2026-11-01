#include "common.h"
#include <algorithm>
#include "appstate.h"
#include "text_io.h"
#include "turn_io.h"
#include "colony_io.h"
#include "part_query_io.h"
#include "race_query_io.h"
#include "alert_io.h"
#include "random_io.h"
#include "record_io.h"

static void WriteRtPassword(APPSESSION &app, std::int32_t lSalt) {
    std::uint8_t rgb[cbWirePassword];

    NATIVEBYTEWRITER(std::as_writable_bytes(std::span(rgb))).FWriteI32Le(lSalt);
    RecordIO().pfnWriteRt(app, rtChgPassword, cbWirePassword, rgb);
}

void WriteOrders(APPSESSION &app, FLEET *lpfl) {
    std::int16_t cord;
    ORDER  *lpord;
    std::byte rgbOrder[cbWireOrder];

    if (lpfl->cord != 0) {
        cord = lpfl->cord;
        lpord = lpfl->lpplord->rgord();
        for (; cord != 0; cord--) {
            if (lpord->grTask == grTaskNone) {
                FEncodeOrderHeader(*lpord, rgbOrder);
                RecordIO().pfnWriteRt(app, rtOrderB, cbWireOrderHeader, rgbOrder);
            } else {
                FEncodeOrder(*lpord, rgbOrder);
                RecordIO().pfnWriteRt(app, rtOrderA, cbWireOrder, rgbOrder);
            }
            lpord++;
        }
    }
    return;
}

// Every saved name has a 31-byte UTF-8 budget. The marker and raw NUL
// are part of the wire image, and the compression budget stays exactly 31.
static bool FAppendWireName(APPSESSION &app, std::span<std::byte> &rgbOut, std::span<const char> szName) {
    const auto pchEnd = std::find(szName.begin(), szName.end(), 0);
    std::size_t cch, cb;
    std::int16_t cOut = 31;
    std::byte rgb[33];

    if (pchEnd == szName.end()) return false;
    cch = static_cast<std::size_t>(pchEnd - szName.begin());
    if (cch > 31 || !FUtf8Valid(std::string_view(szName.data(), cch))) return false;
    const auto cbPacked1 = cch != 0 ? FCompressUserString(app, std::string_view(szName.data(), cch), std::span{rgb}.subspan(1, static_cast<std::size_t>(cOut))) : std::nullopt;
    if (cbPacked1) cOut = static_cast<std::int16_t>(*cbPacked1);
    if (cch != 0 && cbPacked1) {
        rgb[0] = static_cast<std::byte>(cOut);
        cb = static_cast<std::size_t>(cOut) + 1;
    } else {
        NATIVEBYTEWRITER writer(rgb);
        writer.FWriteU8(0);
        writer.FWriteBytes(std::as_bytes(szName.first(cch + 1)));
        cb = writer.CbWritten();
    }
    if (!NATIVEBYTEWRITER(rgbOut).FWriteBytes(std::span(rgb).first(cb))) return false;
    rgbOut = rgbOut.subspan(cb);
    return true;
}

void WriteRtPlr(APPSESSION &app, PLAYER *pplr) {
    std::uint8_t rgb[264];
    std::int16_t i;
    auto pbStore = std::as_writable_bytes(std::span(rgb));
    std::span<std::byte> pb;

    if (pplr->fDead) {
        pplr->det = detAll;
    }
    FEncodePlayer(*pplr, pbStore.first(cbWirePlayer));
    if (pplr->det == detAll) {
        for (i = 15; i >= 0 && pplr->rgmdRelation[i] == 0; i--) {
        }
        i++;
        pb = pbStore.subspan(cbWirePlayerPrefix);
        NATIVEBYTEWRITER relations(pb);
        relations.FWriteU8(static_cast<std::uint8_t>(i));
        relations.FWriteBytes(std::as_bytes(std::span(pplr->rgmdRelation).first(i)));
        pb = pb.subspan(static_cast<std::size_t>(i) + 1);
    } else {
        pb = pbStore.subspan(cbWirePlayerPartial);
    }
    if (!FAppendWireName(app, pb, std::span{pplr->szName}) || !FAppendWireName(app, pb, std::span{pplr->szNames})) {
        FileError(app, idsGameFileAppearsCorruptUnableLoadFile);
        if (FStarsRecoveryActive()) StarsThrowError(starsErrorFile, app.state.idsFileError);
        return;
    }
    RecordIO().pfnWriteRt(app, rtPlr, pbStore.size() - pb.size(), rgb);
}

std::optional<std::size_t> CbEncodeRtShDef(APPSESSION &app, const SHDEF *lpshdef, std::span<std::byte> rgbOut) {
    std::uint8_t rgb[147];
    std::size_t ibName, cb;
    std::span<const char> szHulName;

    if (!FEncodeShDefFixed(*lpshdef, std::as_writable_bytes(std::span(rgb)), ibName)) return std::nullopt;
    if (lpshdef->det == detAll) {
        szHulName = std::span{lpshdef->hul.szClass};
    } else {
        szHulName = std::span{PartQueryIO().pfnHull(lpshdef->hul.ihuldef)->hul.szClass};
    }
    auto pb = std::as_writable_bytes(std::span(rgb)).subspan(ibName);
    if (!FAppendWireName(app, pb, szHulName)) return std::nullopt;
    cb = sizeof(rgb) - pb.size();
    if (!NATIVEBYTEWRITER(rgbOut).FWriteBytes(std::as_bytes(std::span(rgb)).first(cb))) return std::nullopt;
    return cb;
}

void WriteRtShDef(APPSESSION &app, SHDEF *lpshdef) {
    std::byte rgb[147];
    const auto cb = CbEncodeRtShDef(app, lpshdef, rgb);
    if (!cb) {
        FileError(app, idsGameFileAppearsCorruptUnableLoadFile);
        if (FStarsRecoveryActive()) StarsThrowError(starsErrorFile, app.state.idsFileError);
        return;
    }
    RecordIO().pfnWriteRt(app, rtShDef, *cb, rgb);
}

std::int16_t FWriteDataFile(APPSESSION &app, char *pszFileBase, std::int16_t iPlayer, std::int16_t fAppend) {
    PLAYER   plrT;
    std::int16_t  iMax;
    FLEET   *lpflT;
    std::int16_t  fNoAutoTrack;
    BTLPLAN *lpbtlplan;
    std::int16_t  j;
    std::int16_t  i;
    ORDER   *lpord;
    THING   *lpth;
    FLEET   *lpfl;
    std::int16_t  iord;
    SHDEF   *lpshdef;
    THING   *lpthMac;
    THING    th;
    std::int16_t  fRet;
    PLANET  *lpplT;
    SCAN     scan;
    MdTarget mdTarget;
    FLEET   *lpflTarget;
    POINT16  pt;
    std::int32_t  dy;
    std::int16_t  iflT;
    FLEET   *lpflBest;
    std::int16_t  fFoundIdeal;
    std::int32_t  dx;
    std::int32_t  lBest;
    std::int32_t  l;
    PLANET   pl;
    std::byte rgbProd[1024];

    fRet = 1;
    SetVisiblePlanFleet(app, iPlayer);
    if (app.state.gd.fGeneratingTurn && iPlayer != iplrNone) {
        for (i = 0; i < app.state.cFleet; i++) {
            lpfl = app.state.rglpfl[i];
            if (!app.state.rglpfl[i])
                break;
            if (!lpfl->fDead && lpfl->iplr() == iPlayer) {
                for (j = 0; j < 16 && lpfl->rgcsh[j] == 0; j++) {
                }
                if (j == 16) {
                    lpfl->fDead = 1;
                } else {
                    lpord = lpfl->lpplord->rgord();
                    if (lpord->grobj == grobjFleet) {
                        if (FFindNearestObject(app, lpord->pt, static_cast<GrobjClass>(grobjPlanet | mdExact), &scan)) {
                            lpord->grobj = grobjPlanet;
                            lpord->id = scan.idpl;
                        } else {
                            lpord->grobj = grobjOther;
                            lpord->id = 0;
                        }
                    }
                    if (lpord->grTask == grTaskNone && lpfl->cord > 1 && lpord[1].grTask == grTaskPatrol) {
                        lpord->grTask = grTaskPatrol;
                        lpord->tptl().iWarp = lpord[1].tptl().iWarp;
                        lpord->tptl().iDist = lpord[1].tptl().iDist;
                    }
                    if (lpord->grTask == grTaskPatrol && (lpfl->cord <= 1 || lpord[1].grobj != grobjFleet)) {
                        lpflBest = nullptr;
                        lBest = 100000000;
                        fFoundIdeal = 0;
                        if (lpfl->idPlanet == idPlanetDeepSpace && lpfl->cord >= 2 && lpfl->fRepOrders) {
                            pt = lpord[1].pt;
                        } else {
                            pt = lpfl->pt;
                        }
                        lpbtlplan = app.state.rglpbtlplan[lpfl->iPlayer] + lpfl->iplan;
                        mdTarget = static_cast<MdTarget>(lpbtlplan->mdTarget1);
                        for (iflT = 0; iflT < app.state.cFleet; iflT++) {
                            lpflTarget = app.state.rglpfl[iflT];
                            if (!app.state.rglpfl[iflT])
                                break;
                            if (lpflTarget->fInclude && lpflTarget->iPlayer != iPlayer) {
                                dx = static_cast<std::int16_t>((lpflTarget->pt.x - pt.x));
                                dy = static_cast<std::int16_t>((lpflTarget->pt.y - pt.y));
                                l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                                if (((!fFoundIdeal && !lpflTarget->fMark) || (l < lBest && (!fFoundIdeal || !lpflTarget->fMark))) &&
                                    (FMatchTarget(app, lpflTarget, mdTarget, 0) && FAttackPlayer(app, lpfl, lpflTarget->iPlayer))) {
                                    lpflBest = lpflTarget;
                                    lBest = l;
                                    if (!lpflTarget->fMark) {
                                        fFoundIdeal = 1;
                                    }
                                }
                            }
                        }
                        if (fFoundIdeal && !app.state.gd.fTutorial) {
                            lpflBest->fMark = 1;
                        }
                        j = 50 * lpord->tptl().iDist + 50;
                        if (j == 550) {
                            j = 10000;
                        }
                        if (lpflBest && lBest != 0 && lBest <= static_cast<std::int32_t>(static_cast<std::uint32_t>((j * j)))) {
                            if (lpfl->lpplord->iordMax <= lpfl->cord + 1) {
                                lpfl->lpplord = LpplReAlloc(app, lpfl->lpplord, lpfl->cord + 2);
                                lpord = lpfl->lpplord->rgord();
                            }
                            if (lpfl->cord > 1) {
                                std::memmove(lpord + 2, lpord + 1, (lpfl->cord - 1) * sizeof(ORDER));
                            }
                            if (lpfl->cord == 1) {
                                lpord[1] = {};
                                lpord[1].fValidTask = 1;
                                lpord[1].grTask = grTaskPatrol;
                                lpord[1].tptl() = lpord->tptl();
                                if (lpord[1].tptl().iWarp == 0) {
                                    lpord[1].iWarp = IFindIdealWarp(app, lpfl, 0);
                                } else {
                                    lpord[1].iWarp = lpord[1].tsell().iPlrX;
                                }
                                if (lpfl->fRepOrders) {
                                    lpord[2] = *lpord;
                                    lpord[2].iWarp = IFindIdealWarp(app, lpfl, 0);
                                    lpfl->cord++;
                                    lpfl->lpplord->iordMac++;
                                }
                            } else if (lpord[1].tsell().iPlrX == 0) {
                                lpord[1].iWarp = IFindIdealWarp(app, lpfl, 0);
                            } else {
                                lpord[1].iWarp = lpord[1].tsell().iPlrX;
                            }
                            lpord[1].pt = lpflBest->pt;
                            lpord[1].id = lpflBest->id;
                            lpord[1].grobj = grobjFleet;
                            lpfl->cord++;
                            lpfl->lpplord->iordMac++;
                            TurnIO().pfnMessage(app, iPlayer, idmPatrollingHasTargetedIntercept, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lpflBest->id, 0, 0, 0, 0, 0);
                        }
                    }
                    if (lpord->grTask != grTaskXfer && lpfl->cord > 1) {
                        for (iord = 1; iord < lpfl->cord; iord++) {
                            if (lpord[iord].grobj == grobjThing) {
                                lpth = LpthFromId(app, lpord[iord].id);
                                if (!lpth || (lpth->ith() == ithMysteryTrader && !lpth->tht().fInclude()) ||
                                    (lpth->ith() == ithMinefield && !(1 << iPlayer & lpth->thm().grbitPlrNow())) ||
                                    (lpth->ith() == ithWormhole && !lpth->thw().fInclude())) {
                                    if (lpth && lpth->ith() == ithWormhole) {
                                        TurnIO().pfnMessage2(app, lpfl->iPlayer, idmWormholeHeadingHasVanishedOrdersHaveChanged, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                                    } else if (lpth && lpth->ith() == ithMysteryTrader) {
                                        TurnIO().pfnMessage2(app, lpfl->iPlayer, idmMysteryTraderHeadingHasVanishedOrdersHave, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                                    } else if (lpth && lpth->ith() == ithMinefield) {
                                        TurnIO().pfnMessage2(app, lpfl->iPlayer, idmMineFieldHeadingHasVanishedOrdersHave, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                                    }
                                    lpord[iord].grobj = grobjOther;
                                    lpord[iord].id = iord;
                                }
                            } else if (lpord[iord].grobj == grobjFleet) {
                                fNoAutoTrack = lpord[iord].fNoAutoTrack;
                                if (fNoAutoTrack) {
                                    lpord[iord].fNoAutoTrack = 0;
                                }
                                lpflT = LpflFromId(app, lpord[iord].id);
                                if (!lpflT || lpflT->fDead) {
                                    TurnIO().pfnMessage(app, iPlayer, idmSWaypointAppearsHaveDestroyedHasDisappeared, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lpord[iord].id, 0, 0, 0,
                                                0, 0);
                                FixupCoords:
                                    lpord[iord].grobj = grobjOther;
                                    lpord[iord].id = iord;
                                    if (FFindNearestObject(app, lpord[iord].pt, static_cast<GrobjClass>(grobjPlanet | mdExact), &scan)) {
                                        lpord[iord].grobj = grobjPlanet;
                                        lpord[iord].id = scan.idpl;
                                    }
                                } else {
                                    if (lpflT->fInclude)
                                        continue;
                                    if (lpflT->idPlanet != idPlanetDeepSpace && !fNoAutoTrack) {
                                        TurnIO().pfnMessage(app, iPlayer, idmFleetTrackingAppearsHaveDuckedBehindOrders, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lpflT->idPlanet, 0, 0,
                                                    0, 0, 0);
                                        goto FixupCoords;
                                    } else {
                                        TurnIO().pfnMessage(app, iPlayer, idmFleetTrackingAppearsHaveOutrunRangeScanners, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0, 0, 0, 0, 0, 0);
                                        goto FixupCoords;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    MarkPlayersThatSentMsgs(app, iPlayer);
    MarkPlanetsPlayerLost(app, iPlayer);
    if (iPlayer == iplrNone) {
        CchSprintf(app.state.szWork, "%s.hst", pszFileBase);
    } else {
        CchSprintf(app.state.szWork, "%s.m%d", pszFileBase, iPlayer + 1);
    }
    StarsRecoveryScope scopeRecovery;
    try {
        if (fAppend && FAppendFile(app, iPlayer))
            goto LAppend;
        if (!RecordIO().pfnFCreateFile(app, iPlayer == iplrNone ? dtHost : dtTurn, iPlayer, nullptr))
            goto LFail;
LAppend:
        WriteBattles(app, iPlayer);
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (app.state.rgplr[i].fInclude || app.state.rgplr[i].fDead) {
                /* A Claim Adjuster's file carries other players' habitat (their
                   planet values in DrawMineSurvey), which only a full-detail
                   record holds. The original raised the live record to full
                   detail, sending tech, research, traits, production template
                   and relations too, and read rgplr[-1] for the host file. Send a
                   full-detail copy with only the habitat filled in. */
                if (iPlayer != iplrNone && i != iPlayer && !app.state.rgplr[i].fDead && RaceQueryIO().pfnStat(&app.state.rgplr[iPlayer], rsMajorAdv) == raTerra) {
                    plrT = app.state.rgplr[i];
                    std::memset(&plrT.idPlanetHome, 0, reinterpret_cast<std::uint8_t *>(&plrT.szName) - reinterpret_cast<std::uint8_t *>(&plrT.idPlanetHome));
                    std::memcpy(plrT.rgEnvVar, app.state.rgplr[i].rgEnvVar, sizeof(plrT.rgEnvVar));
                    std::memcpy(plrT.rgEnvVarMin, app.state.rgplr[i].rgEnvVarMin, sizeof(plrT.rgEnvVarMin));
                    std::memcpy(plrT.rgEnvVarMax, app.state.rgplr[i].rgEnvVarMax, sizeof(plrT.rgEnvVarMax));
                    plrT.det = detAll;
                    WriteRtPlr(app, &plrT);
                } else {
                    WriteRtPlr(app, &app.state.rgplr[i]);
                }
            }
        }
        if (iPlayer == iplrNone && app.state.lSaltCur != 0) {
            WriteRtPassword(app, app.state.lSaltCur);
        }
        WritePlayerMessages(app, iPlayer);
        i = 0;
        lpplT = app.state.lpPlanets;
        while (i < app.state.cPlanet) {
            if (lpplT->fInclude) {
                if (lpplT->det == detAll) {
                    WritePlanet(app, lpplT, rtPlanet, 0);
                    if (lpplT->lpplprod) {
                        if (!FEncodeProdQueue(std::span(lpplT->lpplprod->rgprod(), lpplT->lpplprod->iprodMac), rgbProd)) goto LFail;
                        RecordIO().pfnWriteRt(app, rtProdQ, lpplT->lpplprod->iprodMac * cbWireProd, rgbProd);
                    }
                } else if (lpplT->det == detObscure) {
                    pl = *lpplT;
                    lpplT->fStarbase = 0;
                    lpplT->det = detSome;
                    WritePlanet(app, lpplT, rtPlanetB, 0);
                    *lpplT = pl;
                } else {
                    WritePlanet(app, lpplT, rtPlanetB, 0);
                }
            }
            i++;
            lpplT++;
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (app.state.rgplr[i].fInclude) {
                lpshdef = app.state.rglpshdef[i];
                for (j = 0; j < 16; j++) {
                    if (!lpshdef[j].fFree && lpshdef[j].fInclude) {
                        WriteRtShDef(app, lpshdef + j);
                    }
                }
            }
        }
        for (i = 0; i < app.state.cFleet; i++) {
            lpflT = app.state.rglpfl[i];
            if (!app.state.rglpfl[i])
                break;
            if (lpflT->fInclude) {
                WriteFleet(app, lpflT);
            }
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (app.state.rgplr[i].fInclude) {
                lpshdef = app.state.rglpshdefSB[i];
                for (j = 0; j < 10; j++) {
                    if (!lpshdef[j].fFree && lpshdef[j].fInclude) {
                        WriteRtShDef(app, lpshdef + j);
                    }
                }
            }
        }
        if (iPlayer != iplrNone && app.state.vlprgScoreX) {
            for (i = 0; i < app.state.game.cPlayer; i++) {
                if (app.state.gd.fGameOverMan || i == iPlayer || app.state.rgplr[i].fDead || (app.state.game.fVisScores && app.state.game.turn >= 20)) {
                    WriteRtScore(app, app.state.vlprgScoreX + i);
                }
            }
        }
        i = 0;
        lpth = app.state.lpThings;
        lpthMac = app.state.lpThings + app.state.cThing;
        for (; lpth < lpthMac; lpth++) {
            if (iPlayer == iplrNone || (iPlayer == lpth->iplr() && lpth->ith() != ithMineralPacket && lpth->ith() != ithMysteryTrader && lpth->ith() != ithWormhole) ||
                (lpth->ith() == ithMinefield && (1 << iPlayer & lpth->thm().grbitPlrNow())) || (lpth->ith() == ithMineralPacket && lpth->thp().fInclude()) ||
                (lpth->ith() == ithMysteryTrader && lpth->tht().fInclude()) || (lpth->ith() == ithWormhole && lpth->thw().fInclude())) {
                i++;
            }
        }
        if (i > 0) {
            std::uint8_t rgbThingCount[2];
            NATIVEBYTEWRITER(std::as_writable_bytes(std::span(rgbThingCount))).FWriteI16Le(i);
            RecordIO().pfnWriteRt(app, rtThing, 2, rgbThingCount);
            lpth = app.state.lpThings;
            lpthMac = app.state.lpThings + app.state.cThing;
            for (; lpth < lpthMac; lpth++) {
                if (iPlayer == iplrNone || (iPlayer == lpth->iplr() && lpth->ith() != ithMineralPacket && lpth->ith() != ithMysteryTrader && lpth->ith() != ithWormhole) ||
                    (lpth->ith() == ithMinefield && (1 << iPlayer & lpth->thm().grbitPlrNow())) || (lpth->ith() == ithMineralPacket && lpth->thp().fInclude()) ||
                    (lpth->ith() == ithMysteryTrader && lpth->tht().fInclude()) || (lpth->ith() == ithWormhole && lpth->thw().fInclude())) {
                    if (iPlayer == iplrNone) {
                        WriteRtThing(app, lpth);
                    } else {
                        /* A player's file keeps only that player's bit of each
                           per-player mask, and not the part the Mystery Trader
                           carries. The original wrote the whole record, telling
                           every player who else had seen these things. */
                        th = *lpth;
                        switch (th.ith()) {
                        case ithMinefield:
                            th.thm().SetGrbitPlr(th.thm().grbitPlr() & (1 << iPlayer));
                            th.thm().SetGrbitPlrNow(th.thm().grbitPlrNow() & (1 << iPlayer));
                            break;
                        case ithWormhole:
                            th.thw().SetGrbitPlr(th.thw().grbitPlr() & (1 << iPlayer));
                            th.thw().SetGrbitPlrTrav(th.thw().grbitPlrTrav() & (1 << iPlayer));
                            break;
                        case ithMysteryTrader:
                            th.tht().SetGrbitPlr(th.tht().grbitPlr() & (1 << iPlayer));
                            th.tht().SetGrbitTrader(grbitTraderNone);
                            break;
                        }
                        WriteRtThing(app, &th);
                    }
                }
            }
        }
        if (iPlayer == iplrNone) {
            i = 0;
            iMax = app.state.game.cPlayer;
        } else {
            i = iPlayer;
            iMax = iPlayer + 1;
        }
        for (; i < iMax; i++) {
            lpbtlplan = app.state.rglpbtlplan[i];
            j = 0;
            while (j < app.state.rgcbtlplan[i]) {
                WriteBattlePlan(app, lpbtlplan, 0);
                j++;
                lpbtlplan++;
            }
        }
        WriteRtEof(app, app.state.game.turn);
        RecordIO().pfnStreamClose(app);
    } catch (const StarsError &) {
        goto LFail;
    }
    goto FreeUp;
LFail:
    app.state.idPlayer = iPlayer;
    if (fAppend) {
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableUpdateTurnFile, nullptr), win32::MB_ICONHAND);
    } else if (iPlayer != iplrNone) {
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableCreateNewTurnFile, nullptr), win32::MB_ICONHAND);
    } else {
        app.state.idPlayer = iplrNone;
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableCreateHostFile, nullptr), win32::MB_ICONHAND);
    }
    app.state.idPlayer = iplrNone;
    fRet = 0;
    goto FreeUp;

FreeUp:
    SetVisiblePlanFleet(app, iplrNone);
    return fRet;
}

std::int16_t FAppendFile(APPSESSION &app, std::int16_t iPlayer) {
    if (!FMarkFile(app, static_cast<DtFileType>(8195), iPlayer, mdMarkMulti, 1)) {
        return 0;
    }
    WriteBOF(app, iPlayer, 3, 1);
    return 1;
}

void WriteBattles(APPSESSION &app, std::int16_t iPlayer) {
    std::int16_t  ctok;
    std::int16_t  cbRec;
    PLANET  *lppl;
    std::int16_t  i;
    FLEET   *lpfl;
    std::int16_t  cbT;
    std::uint16_t fPlayerCur;
    std::uint8_t *lpbtlrec;
    std::uint8_t *lpbBattle;
    HB      *lphb;
    std::uint8_t *lpbtldata;
    std::int16_t  cb;
    std::int16_t  iplr;

    cbT = 0;
    if (iPlayer != iplrNone && app.state.lpbBattleLog != app.state.lpbBattleCur) {
        lphb = app.state.rglphb[11];
        lpbBattle = reinterpret_cast<std::uint8_t *>(lphb) + (sizeof(HB) + 2);
        fPlayerCur = 1 << iPlayer;
        while (lphb) {
            for (lpbtldata = static_cast<std::uint8_t *>(lpbBattle); lphb->ibTop <= sizeof(HB) || BdvFromLp(app, lpbtldata).id() == 0xffff; lpbtldata = static_cast<std::uint8_t *>(lpbBattle)) {
                lphb = lphb->lphbNext;
                if (!lphb) {
                    return;
                }
                lpbBattle = reinterpret_cast<std::uint8_t *>(lphb) + (sizeof(HB) + 2);
            }
            if (BdvFromLp(app, lpbtldata).grfPlr() & fPlayerCur) {
                for (i = 0; i < app.state.game.cPlayer; i++) {
                    if (i != iPlayer && !app.state.rgplr[i].fInclude && (1 << i & BdvFromLp(app, lpbtldata).grfPlr())) {
                        app.state.rgplr[i].fInclude = 1;
                        app.state.rgplr[i].det = detSome;
                    }
                }
                for (i = 0; i < BdvFromLp(app, lpbtldata).ctok(); i++) {
                    if (BdvFromLp(app, lpbtldata).rgtok(i).iplr() != iPlayer) {
                        if (BdvFromLp(app, lpbtldata).rgtok(i).grobj() == grobjPlanet) {
                            iplr = BdvFromLp(app, lpbtldata).rgtok(i).iplr();
                            lppl = LpplFromId(app, BdvFromLp(app, lpbtldata).rgtok(i).id());
                            if (!app.state.rglpshdefSB[iplr][BdvFromLp(app, lpbtldata).rgtok(i).ishdef() - 16].fInclude) {
                                app.state.rglpshdefSB[iplr][BdvFromLp(app, lpbtldata).rgtok(i).ishdef() - 16].fInclude = 1;
                                app.state.rgplr[iplr].cshdefSB++;
                            }
                            app.state.rglpshdefSB[iplr][BdvFromLp(app, lpbtldata).rgtok(i).ishdef() - 16].det = detAll;
                        } else {
                            lpfl = LpflFromId(app, BdvFromLp(app, lpbtldata).rgtok(i).id());
                            if (lpfl->iPlayer != iPlayer && !app.state.rgplr[lpfl->iPlayer].fInclude) {
                                app.state.rgplr[lpfl->iPlayer].fInclude = 1;
                                app.state.rgplr[lpfl->iPlayer].det = detSome;
                            }
                            if (!app.state.rglpshdef[lpfl->iPlayer][BdvFromLp(app, lpbtldata).rgtok(i).ishdef()].fInclude) {
                                app.state.rglpshdef[lpfl->iPlayer][BdvFromLp(app, lpbtldata).rgtok(i).ishdef()].fInclude = 1;
                                app.state.rgplr[lpfl->iPlayer].cShDef = app.state.rgplr[lpfl->iPlayer].cShDef + 1;
                            }
                            app.state.rglpshdef[lpfl->iPlayer][BdvFromLp(app, lpbtldata).rgtok(i).ishdef()].det = detAll;
                            if (!lpfl->fDead) {
                                if (!lpfl->fInclude) {
                                    app.state.rgplr[lpfl->iPlayer].cFleet = app.state.rgplr[lpfl->iPlayer].cFleet + 1;
                                    lpfl->fInclude = 1;
                                    lpfl->det = detNone;
                                }
                                if (lpfl->det < detSome) {
                                    lpfl->det = detSome;
                                }
                            }
                        }
                    }
                }
                if (BdvFromLp(app, lpbtldata).idPlanet() != idPlanetNone) {
                    lppl = LpplFromId(app, BdvFromLp(app, lpbtldata).idPlanet());
                    MarkPlanet(app, lppl, iPlayer, detMinimal);
                }
                if (BdvFromLp(app, lpbtldata).cbData() < 0x400) {
                    RecordIO().pfnWriteRt(app, rtBtlData, BdvFromLp(app, lpbtldata).cbData(), lpbBattle);
                    lpbBattle += BdvFromLp(app, lpbtldata).cbData();
                } else {
                    cb = BdvFromLp(app, lpbtldata).ctok() * 29 + 14;
                    lpbtlrec = static_cast<std::uint8_t *>((lpbBattle + cb));
                    if (cb < 1024) {
                        RecordIO().pfnWriteRt(app, rtBtlData, cb, lpbBattle);
                        lpbBattle += cb;
                    } else {
                        ctok = 34;
                        ctok = ctok >= BdvFromLp(app, lpbtldata).ctok() ? BdvFromLp(app, lpbtldata).ctok() : ctok;
                        if (ctok > BdvFromLp(app, lpbtldata).ctok()) {
                            ctok = BdvFromLp(app, lpbtldata).ctok();
                        }
                        RecordIO().pfnWriteRt(app, rtBtlData, ctok * 29 + 14, lpbBattle);
                        lpbBattle += 14 + 29 * ctok;
                        ctok = BdvFromLp(app, lpbtldata).ctok() - ctok;
                        while (ctok > 0) {
                            if (static_cast<std::uint16_t>(ctok) > 35) {
                                RecordIO().pfnWriteRt(app, rtContinue, 1015, lpbBattle);
                                lpbBattle += 1015;
                                ctok -= 35;
                            } else {
                                RecordIO().pfnWriteRt(app, rtContinue, ctok * 29, lpbBattle);
                                lpbBattle += 29 * ctok;
                                ctok = 0;
                            }
                        }
                    }
                    cb = BdvFromLp(app, lpbtldata).cbData() - 14 - BdvFromLp(app, lpbtldata).ctok() * 29;
                    if (cb < 1024) {
                        RecordIO().pfnWriteRt(app, rtContinue, cb, lpbtlrec);
                        lpbBattle += cb;
                    } else {
                        while (cb != 0) {
                            cbRec = BevFromLp(app, lpbtlrec).ctok() * 8 + 6;
                            if (cbRec >= 1024) {
                                cb -= cbRec;
                                for (; cbRec >= 1024; cbRec -= 1023) {
                                    RecordIO().pfnWriteRt(app, rtContinue, 1023, lpbtlrec);
                                    lpbtlrec = static_cast<std::uint8_t *>((static_cast<std::uint8_t *>(lpbtlrec) + 0x3ff));
                                }
                                if (cbRec != 0) {
                                    RecordIO().pfnWriteRt(app, rtContinue, cbRec, lpbtlrec);
                                    lpbtlrec = static_cast<std::uint8_t *>((static_cast<std::uint8_t *>(lpbtlrec) + cbRec));
                                }
                            } else {
                                cbT = 0;
                                do {
                                    cbT += cbRec;
                                    lpbtlrec = static_cast<std::uint8_t *>((static_cast<std::uint8_t *>(lpbtlrec) + cbRec));
                                    cb -= cbRec;
                                    if (cb != 0) {
                                        cbRec = BevFromLp(app, lpbtlrec).ctok() * 8 + 6;
                                    }
                                } while (cb != 0 && cbT + cbRec < 1024);
                                RecordIO().pfnWriteRt(app, rtContinue, cbT, lpbBattle);
                            }
                            lpbBattle = static_cast<std::uint8_t *>(lpbtlrec);
                        }
                    }
                }
            } else {
                lpbBattle += BdvFromLp(app, lpbtldata).cbData();
            }
        }
    }
    return;
}

void WritePlanet(APPSESSION &app, PLANET *lppl, RecordType rt, std::int16_t fHistory) {
    std::uint8_t  bMask;
    std::uint8_t  rgb[80];
    std::uint8_t *pbBase;
    std::int16_t  i;
    std::uint8_t *pb;

    std::fill_n(rgb, 80, 0);
    PpvFromPrefix(rgb, sizeof(rgb)).SetId(lppl->id);
    PpvFromPrefix(rgb, sizeof(rgb)).SetIPlayer(lppl->iPlayer);
    PpvFromPrefix(rgb, sizeof(rgb)).SetDet(lppl->det);
    if (rt == rtPlanetB && lppl->det > detSome) {
        PpvFromPrefix(rgb, sizeof(rgb)).SetDet(!fHistory ? 4 : 3);
    }
    PpvFromPrefix(rgb, sizeof(rgb)).SetFInclude(lppl->fInclude);
    PpvFromPrefix(rgb, sizeof(rgb)).SetFStarbase(lppl->fStarbase);
    PpvFromPrefix(rgb, sizeof(rgb)).SetFHomeworld(lppl->fHomeworld);
    PpvFromPrefix(rgb, sizeof(rgb)).SetFFirstYear(lppl->fFirstYear);
    PpvFromPrefix(rgb, sizeof(rgb)).SetFRouting(lppl->idRoute != 0);
    pbBase = rgb + cbWirePlanet;
    pb = pbBase;
    if (PpvFromPrefix(rgb, sizeof(rgb)).det() <= detMinimal)
        goto LFinishBRecord;

    pb = pbBase + 1;
    bMask = 3;
    i = 0;
    while (i < 3) {
        if (lppl->rgpctMinLevel[i] > 0) {
            *pbBase |= bMask & 0x55;
            *pb++ = lppl->rgpctMinLevel[i];
        }
        i++;
        bMask *= 4;
    }
    i = 0;
    while (i < 3) {
        *pb = lppl->rgMinConc[i];
        i++;
        pb++;
    }
    for (i = 0; i < 3; i++) {
        *pb++ = lppl->rgEnvVar[i];
        if (lppl->rgEnvVar[i] != lppl->rgEnvVarOrig[i]) {
            PpvFromPrefix(rgb, sizeof(rgb)).SetFIncEVO(1);
        }
    }
    if (PpvFromPrefix(rgb, sizeof(rgb)).fIncEVO() != 0) {
        for (i = 0; i < 3; i++) {
            *pb++ = lppl->rgEnvVarOrig[i];
        }
    }
    if (lppl->iPlayer != iplrNone) {
        NATIVEBYTEWRITER(std::as_writable_bytes(std::span(pb, 2))).FWriteU16Le(lppl->uGuesses());
        pb += 2;
    }
    if (PpvFromPrefix(rgb, sizeof(rgb)).det() <= detSome)
        goto LFinishBRecord;

    pbBase = pb;
    pb++;
    bMask = 3;
    i = 0;
    while (i < 4) {
        if ((i != 3 || lppl->det >= detAll) && lppl->rgwtMin[i] > 0) {
            if (lppl->rgwtMin[i] <= 255) {
                *pbBase |= bMask & 0x55;
                *pb++ = lppl->rgwtMin[i];
            } else if (lppl->rgwtMin[i] > 65535) {
                *pbBase |= bMask;
                NATIVEBYTEWRITER(std::as_writable_bytes(std::span(pb, 4))).FWriteI32Le(lppl->rgwtMin[i]);
                pb += 4;
            } else {
                *pbBase |= bMask & 0xaa;
                NATIVEBYTEWRITER(std::as_writable_bytes(std::span(pb, 2))).FWriteU16Le(LOWORD(lppl->rgwtMin[i]));
                pb += 2;
            }
        }
        i++;
        bMask *= 4;
    }
    if (*pbBase == 0) {
        pb = pbBase;
    } else {
        PpvFromPrefix(rgb, sizeof(rgb)).SetFIncSurfMin(1);
    }
    if (rt == rtPlanetB) {
    LFinishBRecord:
        if (lppl->fStarbase) {
            *pb = lppl->isb;
            pb++;
        }
        if (fHistory) {
            NATIVEBYTEWRITER(std::as_writable_bytes(std::span(pb, 2))).FWriteI16Le(lppl->turn);
            pb += 2;
        }
        RecordIO().pfnWriteRt(app, rtPlanetB, pb - rgb, rgb);
    } else {
        PpvFromPrefix(rgb, sizeof(rgb)).SetFIsArtifact(lppl->fArtifact);
        if ((lppl->iPlayer != iplrNone && (lppl->iDeltaPop != 0 || lppl->fNoResearch)) ||
            (lppl->cMines != 0 || lppl->cFactories != 0 || lppl->cDefenses != 0 || lppl->iScanner != 31)) {
            PpvFromPrefix(rgb, sizeof(rgb)).SetFIncImp(1);
            const auto rgbImp = lppl->rgbImp();
            std::memmove(pb, rgbImp.data(), 8);
            pb += 8;
        }
        if (lppl->iPlayer != iplrNone) {
            if (lppl->fStarbase) {
                NATIVEBYTEWRITER(std::as_writable_bytes(std::span(pb, 4))).FWriteI32Le(lppl->lStarbase());
                pb += 4;
            }
            if (lppl->idRoute != 0) {
                NATIVEBYTEWRITER(std::as_writable_bytes(std::span(pb, 2))).FWriteU16Le(lppl->wRouting());
                pb += 2;
            }
        }
        RecordIO().pfnWriteRt(app, rtPlanet, pb - rgb, rgb);
    }
}

void WriteFleet(APPSESSION &app, FLEET *lpfl) {
    std::uint8_t rgb[134];
    std::int16_t i;
    std::int32_t wt = 0;
    bool fPartial = lpfl->det < detAll;
    std::size_t cb;

    if (fPartial) {
        for (i = 0; i < 16; i++) {
            if (lpfl->rgcsh[i] > 0) {
                wt += static_cast<std::uint32_t>((lpfl->rgcsh[i] * static_cast<std::uint32_t>(app.state.rglpshdef[lpfl->iPlayer][i].hul.wtEmpty)));
            }
        }
        for (i = 0; i <= 3; i++) {
            wt += lpfl->rgwtMin[i];
        }
    }
    if (!FEncodeRtFleet(*lpfl, wt, std::as_writable_bytes(std::span(rgb)), cb)) {
        FileError(app, idsGameFileAppearsCorruptUnableLoadFile);
        return;
    }
    RecordIO().pfnWriteRt(app, fPartial ? rtFleetB : rtFleetA, cb, rgb);
    if (!fPartial) {
        WriteOrders(app, lpfl);
        if (lpfl->lpszName) WriteRtString(app, lpfl->lpszName);
    }
}

void WriteRtString(APPSESSION &app, const char *lpsz) {
    std::byte rgb[33];
    std::size_t cch;
    std::span<std::byte> pb = rgb;

    if (lpsz && *lpsz != 0) {
        cch = std::strlen(lpsz);
        if (!FAppendWireName(app, pb, std::span(lpsz, cch + 1))) {
            FileError(app, idsGameFileAppearsCorruptUnableLoadFile);
            if (FStarsRecoveryActive()) StarsThrowError(starsErrorFile, app.state.idsFileError);
            return;
        }
        RecordIO().pfnWriteRt(app, rtString, sizeof(rgb) - pb.size(), rgb);
    }
}

void MarkFleet(APPSESSION &app, FLEET *lpfl, DetType det) {
    std::int16_t i;
    SHDEF  *lpshdef;

    if (!lpfl->fInclude) {
        lpshdef = app.state.rglpshdef[lpfl->iPlayer];
        lpfl->fInclude = 1;
        lpfl->det = detNone;
        lpfl->SetFdirValid(1);
        app.state.rgplr[lpfl->iPlayer].cFleet = app.state.rgplr[lpfl->iPlayer].cFleet + 1;
        for (i = 0; i < 16; i++) {
            if (lpfl->rgcsh[i] != 0) {
                lpshdef[i].fInclude = 1;
            }
        }
    }
    if (lpfl->det < det) {
        lpfl->det = det;
    }
    return;
}

void WriteBattlePlan(APPSESSION &app, BTLPLAN *lpbtlplan, std::int16_t fLog) {
    // A raw name adds a length marker before its fixed 32-byte field.
    std::uint8_t  rgb[cbWireBattlePlan + 32 + 1];
    std::span<std::byte> pb;

    FEncodeBattlePlanPrefix(*lpbtlplan, std::as_writable_bytes(std::span(rgb)).first(cbWireBattlePlan));
    if (lpbtlplan->fDelete) {
        pb = std::as_writable_bytes(std::span(rgb)).subspan(2);
    } else {
        pb = std::as_writable_bytes(std::span(rgb)).subspan(4);
        if (!FAppendWireName(app, pb, std::span{lpbtlplan->szName})) {
            FileError(app, idsGameFileAppearsCorruptUnableLoadFile);
            if (FStarsRecoveryActive()) StarsThrowError(starsErrorFile, app.state.idsFileError);
            return;
        }
    }
    if (fLog) {
        WriteMemRt(app, rtBtlPlan, sizeof(rgb) - pb.size(), rgb);
    } else {
        RecordIO().pfnWriteRt(app, rtBtlPlan, sizeof(rgb) - pb.size(), rgb);
    }
    return;
}

void MarkPlanet(APPSESSION &app, PLANET *lppl, std::int16_t iPlr, DetType det) {
    SHDEF *lpshdef;

    if (!lppl->fInclude) {
        lppl->fInclude = 1;
        lppl->det = detNone;
        app.state.rgplr[iPlr].cPlanet++;
    }
    if (lppl->det < det) {
        lppl->det = det;
    }
    if (lppl->iPlayer != iplrNone && !app.state.rgplr[lppl->iPlayer].fInclude) {
        app.state.rgplr[lppl->iPlayer].fInclude = 1;
        app.state.rgplr[lppl->iPlayer].det = detSome;
    }
    if (det != detObscure && lppl->iPlayer != iplrNone && lppl->fStarbase) {
        lpshdef = app.state.rglpshdefSB[lppl->iPlayer] + lppl->isb;
        if (!lpshdef->fInclude) {
            lpshdef->fInclude = 1;
            lpshdef->det = detNone;
            app.state.rgplr[lppl->iPlayer].cshdefSB = app.state.rgplr[lppl->iPlayer].cshdefSB + 1;
        }
        if (lpshdef->det < detSome) {
            lpshdef->det = detSome;
        }
    }
    return;
}

void SetSzWorkFromDt(APPSESSION &app, DtFileType dt, std::int16_t iPlayer) {
    char   *pchSlash;
    std::int16_t c;
    char   *pchDot;

    pchDot = std::strrchr(app.state.szBase, 46);
    if (pchDot) {
        pchSlash = std::strrchr(app.state.szBase, chDirSep);
        if (!pchSlash || pchSlash < pchDot) {
            *pchDot = 0;
        }
    }
    c = CchSprintf(app.state.szWork, "%s.", app.state.szBase);
    switch (dt) {
    case dtXY:
    default:
        std::strcat(app.state.szWork, "xy");
        break;
    case dtHost:
        std::strcat(app.state.szWork, "hst");
        break;
    case dtLog:
    case dtTurn:
    case dtHist:
        CchSprintf(std::span{app.state.szWork}.subspan(static_cast<std::size_t>(c)), "%c%d", dt == dtLog ? 120 : dt == dtHist ? 104 : 109, iPlayer + 1);
    }
    return;
}

std::int16_t FCreateFile(APPSESSION &app, DtFileType dt, std::int16_t iPlayer, char *szForceName) {
    char    *psz;

    if (szForceName) {
        psz = szForceName;
    } else {
        SetSzWorkFromDt(app, dt, iPlayer);
        psz = app.state.szWork;
    }
    StarsRecoveryScope scopeRecovery;
    try {
        StarsFileGuard closeStream(app.state.hf);
        RecordIO().pfnStreamOpen(app, psz, mdCreate);
        WriteBOF(app, iPlayer, dt, 0);
        closeStream.Release();
        return 1;
    } catch (const StarsError &) {
        return 0;
    }
}

static void WriteRtBof(APPSESSION &app, const RTBOF &rtbof) {
    std::byte rgb[cbWireBof];

    FEncodeBof(rtbof, rgb);
    RecordIO().pfnWriteRt(app, rtBOF, cbWireBof, rgb);
}

void WriteBOF(APPSESSION &app, std::int16_t iPlayer, std::int16_t dt, std::int16_t fMulti) {
    RTBOF rtbof;

    rtbof = {};
    std::strncpy(rtbof.rgid, "J3J3", 4);
    rtbof.lidGame = app.state.game.lid;
    rtbof.wGen = app.state.game.wGen;
    rtbof.verInc = 0;
    // 2.85 identifies UTF-8 text; older files have ambiguous byte encodings.
    rtbof.verMinor = 85;
    rtbof.verMajor = 2;
    rtbof.turn = app.state.game.turn;
    rtbof.fCrippled = 0;
    rtbof.iPlayer = iPlayer;
    rtbof.lSaltTime = static_cast<std::int16_t>((LOWORD(DwTickCount()) + RandomIO().pfnRandom(app, 2000)));
    rtbof.dt = dt;
    rtbof.fDone = app.state.gd.fSubmit;
    rtbof.fInUse = app.state.gd.fHostMode;
    rtbof.fGameOverMan = dt == 2 && app.state.gd.fGameOverMan != 0;
    WriteRtBof(app, rtbof);
    return;
}

std::int16_t FMarkFile(APPSESSION &app, DtFileType dt, std::int16_t iPlayer, MdMark mdMark, std::int16_t f) {
    StringId ids;
    RTBOF    rtbof;
    std::int16_t  fChange;
    std::int16_t  fSuccess;
    std::int16_t  fSilentSav;
    std::int32_t  lSeedSav2;
    std::int32_t  lSeedSav1;

    fSilentSav = app.state.fFileErrSilent;
    fSuccess = 0;
    ids = idsUniverseDefinitionFileSeemsMissingCorrupt;
    SetSzWorkFromDt(app, static_cast<DtFileType>(dt & 0xff), iPlayer);
    StarsRecoveryScope scopeRecovery;
    try {
        app.state.fFileErrSilent = 1;
        RecordIO().pfnStreamOpen(app, app.state.szWork, mdReadWrite);
        app.state.fFileErrSilent = fSilentSav;
        ids = idsGameFileAppearsCorruptUnableLoadFile;
        RecordIO().pfnReadRt(app);
        if (app.state.hdrCur.rt != rtBOF) {
            FileError(app, idsFileDoesBelongVersionStars);
            goto LBadFile;
        }
        if (BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMajor() < 2 || (BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMajor() == 2 && BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMinor() < 85)) {
            FileError(app, idsSorryFileCreatedOlderVersionStarsIncompatible);
            goto LBadFile;
        }
        if (BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMajor() > 2 || (BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMajor() == 2 && BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMinor() > 85)) {
            FileError(app, idsFileCreatedNewerVersionStarsMustUpgrade);
            goto LBadFile;
        }
        FDecodeBof(std::as_bytes(std::span(app.state.rgbCur)), rtbof);
        if (app.state.game.lid == 0)
            goto LBadFile;
        if (rtbof.lidGame != app.state.game.lid) {
            FileError(app, idsFileGame);
            goto LBadFile;
        }
        fChange = 0;
        switch (mdMark) {
        case mdMarkInUse:
            if (rtbof.fInUse != f) {
                rtbof.fInUse = f;
                fChange = 1;
            }
            break;
        case mdMarkDone:
            if (rtbof.fDone != f) {
                rtbof.fDone = f;
                fChange = 1;
            }
            break;
        case mdMarkMulti:
            if (rtbof.fMulti != f) {
                rtbof.fMulti = f;
                fChange = 1;
            }
            break;
        case mdMarkAi:
            do {
                GetFileSeeds(app, &lSeedSav1, &lSeedSav2);
                RecordIO().pfnReadRt(app);
            } while (app.state.hdrCur.rt != rtPlr || PlvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).iPlayer() != iPlayer);
            if (PlvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fAi() != f) {
                if (PlvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fAi() != 0) {
                    if (PlvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).idAi() != idAiMaid)
                        goto LBadFile;
                    PlvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).SetFAi(0);
                } else {
                    PlvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).SetFAi(1);
                    PlvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).SetIdAi(idAiMaid);
                }
                PlvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).SetLSalt(~PlvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).lSalt());
                LSeekFile(app.state.hf, static_cast<std::int16_t>(-(app.state.hdrCur.cb + 2)), NATIVESEEKORIGIN::current);
                SetFileSeeds(app, lSeedSav1, lSeedSav2);
                RecordIO().pfnWriteRt(app, rtPlr, app.state.hdrCur.cb, app.state.rgbCur);
                fChange = dt == dtTurn;
                rtbof.fDone = 0;
            }
            break;
        }
        if (fChange) {
            LSeekFile(app.state.hf, 0, NATIVESEEKORIGIN::begin);
            WriteRtBof(app, rtbof);
        }
        fSuccess = 1;
LBadFile:
        if ((dt & 0x2000) && fSuccess) {
            LSeekFile(app.state.hf, 0, NATIVESEEKORIGIN::end);
        } else {
            RecordIO().pfnStreamClose(app);
        }
        return fSuccess;
    } catch (const StarsError &) {
        app.state.fFileErrSilent = fSilentSav;
        if (ids != idsUniverseDefinitionFileSeemsMissingCorrupt) {
            FileError(app, ids);
        }
        RecordIO().pfnStreamClose(app);
        return 0;
    }
}

void WriteRtThing(APPSESSION &app, const THING *pth) {
    std::byte rgb[cbWireThing];
    FEncodeThing(*pth, rgb);
    RecordIO().pfnWriteRt(app, rtThing, cbWireThing, rgb);
}

void WriteRtScore(APPSESSION &app, const SCOREX *psx) {
    std::byte rgb[cbWireScoreX];

    FEncodeScoreX(*psx, rgb);
    RecordIO().pfnWriteRt(app, rtScore, cbWireScoreX, rgb);
}

void WriteRtEof(APPSESSION &app, std::uint16_t w) {
    std::byte rgb[2];

    NATIVEBYTEWRITER(rgb).FWriteU16Le(w);
    RecordIO().pfnWriteRt(app, rtEOF, 2, rgb);
}

void WriteRt(APPSESSION &app, RecordType rt, std::int16_t cb, void *rg) {
    HDR hdr;
    std::byte rgbHdr[cbWireHdr];

    if (cb != 0)
        std::memmove(app.state.rgbCur, rg, cb);
    if (rt == rtBOF) {
        SetFileXorStream(app, BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).lidGame(), BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).lSaltTime(), BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).turn(), BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).iPlayer(),
                         BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fCrippled());
    } else if (rt != rtEOF) {
        XorFileBuf(app, app.state.rgbCur, cb);
    }
    hdr.cb = cb;
    hdr.rt = rt;
    FEncodeHdr(hdr, rgbHdr);
    RgToStream(app, std::as_bytes(std::span{rgbHdr}).first(static_cast<std::size_t>(cbWireHdr)));
    RgToStream(app, std::as_bytes(std::span{app.state.rgbCur}).first(static_cast<std::size_t>(cb)));
    return;
}

void RgToStream(APPSESSION &app, std::span<const std::byte> rg) {
    if (!rg.empty() && CbWriteFile(app.state.hf, rg) != rg.size()) {
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsErrorWritingFile, nullptr), win32::MB_ICONHAND);
        StarsThrowError(starsErrorFile, idsErrorWritingFile);
    }
    return;
}

void SetVisiblePlanFleet(APPSESSION &app, std::int16_t iPlr) {
    SetVisPFInit(app, iPlr);
    if (iPlr == iplrNone) {
        app.state.rgplr[0].cPlanet = app.state.game.cPlanMax;
    } else {
        if (iPlr != iplrNone) {
            UpdateProgressGauge(app, progressStep1);
        }
        SetVisPFFleets(app, iPlr);
        if (iPlr != iplrNone) {
            UpdateProgressGauge(app, progressStep1);
        }
        SetVisPFPlanets(app, iPlr);
        if (iPlr != iplrNone) {
            UpdateProgressGauge(app, progressStep1);
        }
        SetVisPFThings(app, iPlr);
        SetVisPFFinish(app, iPlr);
    }
    return;
}

void SetVisPFInit(APPSESSION &app, std::int16_t iPlr) {
    PLANET       *lpplMac;
    std::uint16_t      detNew;
    PLANET       *lppl;
    std::int16_t       j;
    FLEET        *lpfl;
    THING        *lpth;
    std::int16_t       ifl;
    std::int16_t       i;
    THING        *lpthMac;
    RaceAttribute raMajor;
    std::uint16_t      grbitPlr;
    std::int16_t       iSteal;

    // Host visibility has no player race; every packet is included below.
    raMajor = static_cast<RaceAttribute>(iPlr == iplrNone ? raNone : RaceQueryIO().pfnStat(&app.state.rgplr[iPlr], rsMajorAdv));
    grbitPlr = iPlr == iplrNone ? 0 : 1 << iPlr;
    for (i = 0; i < app.state.game.cPlayer; i++) {
        app.state.rgplr[i].cPlanet = 0;
    }
    for (i = 0; i < app.state.game.cPlayer; i++) {
        if (iPlr == iplrNone || iPlr == i || app.state.rgplr[i].fDead) {
            app.state.rgplr[i].fInclude = 1;
            app.state.rgplr[i].det = detAll;
        } else {
            app.state.rgplr[i].fInclude = 0;
        }
        app.state.rgplr[i].cFleet = 0;
        app.state.rgplr[i].cShDef = 0;
        app.state.rgplr[i].cshdefSB = 0;
        for (j = 0; j < 16; j++) {
            if ((iPlr == iplrNone || iPlr == i) && !app.state.rglpshdef[i][j].fFree) {
                app.state.rglpshdef[i][j].fInclude = 1;
                app.state.rglpshdef[i][j].det = detAll;
                app.state.rglpshdef[i][j].cExist = 0;
                app.state.rgplr[i].cShDef++;
            } else {
                app.state.rglpshdef[i][j].fInclude = 0;
            }
        }
        for (j = 0; j < 10; j++) {
            if ((iPlr == iplrNone || iPlr == i) && !app.state.rglpshdefSB[i][j].fFree) {
                app.state.rglpshdefSB[i][j].fInclude = 1;
                app.state.rglpshdefSB[i][j].det = detAll;
                app.state.rglpshdefSB[i][j].cExist = 0;
                app.state.rgplr[i].cshdefSB++;
            } else {
                app.state.rglpshdefSB[i][j].fInclude = 0;
            }
        }
    }
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (iPlr == iplrNone || iPlr == lppl->iPlayer) {
            lppl->fInclude = 1;
            lppl->det = detAll;
            if (iPlr != iplrNone) {
                app.state.rgplr[iPlr].cPlanet++;
            }
            if (lppl->fStarbase) {
                app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].cExist = app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].cExist + 1;
            }
        } else {
            lppl->fInclude = 0;
        }
    }
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        lpfl->SetFdirValid(0);
        lpfl->fMark = 0;
        if ((iPlr == iplrNone || iPlr == lpfl->iPlayer) && !lpfl->fDead) {
            lpfl->fInclude = 1;
            lpfl->det = detAll;
            app.state.rgplr[lpfl->iPlayer].cFleet = app.state.rgplr[lpfl->iPlayer].cFleet + 1;
            for (j = 0; j < 16; j++) {
                if (lpfl->rgcsh[j] != 0) {
                    app.state.rglpshdef[lpfl->iPlayer][j].cExist = app.state.rglpshdef[lpfl->iPlayer][j].cExist + lpfl->rgcsh[j];
                }
            }
            if (iPlr != iplrNone && lpfl->idPlanet != idPlanetDeepSpace) {
                lppl = app.state.lpPlanets + lpfl->idPlanet;
                detNew = GetCachedFleetScannerRange(app, lpfl, nullptr, nullptr, &iSteal) < 0 ? 1 : 3;
                if (iSteal >= 2) {
                    detNew = 4;
                }
                if (lpfl->fHereAllTurn && lppl->iPlayer == iplrNone && lpfl->lpplord->rgord()[0].grTask == grTaskMine && ColonyIO().pfnFleetMining(app, lpfl) > 0) {
                    detNew = 4;
                }
                MarkPlanet(app, lppl, iPlr, static_cast<DetType>(detNew));
            }
        } else {
            lpfl->fInclude = 0;
        }
    }
    lpth = app.state.lpThings;
    lpthMac = app.state.lpThings + app.state.cThing;
    for (; lpth < lpthMac; lpth++) {
        switch (lpth->ith()) {
        case ithMineralPacket:
            if (iPlr == iplrNone || raMajor == raMassAccel) {
                lpth->thp().SetFInclude(1);
                if (app.state.rgplr[lpth->iplr()].fInclude)
                    break;
                app.state.rgplr[lpth->iplr()].fInclude = 1;
                app.state.rgplr[lpth->iplr()].det = detSome;
                break;
            }
            lpth->thp().SetFInclude(0);
            break;
        case ithWormhole:
            lpth->thw().SetFInclude(iPlr == iplrNone);
            break;
        case ithMysteryTrader:
            lpth->tht().SetFInclude(1);
            break;
        case ithMinefield:
            if ((lpth->thm().grbitPlrNow() & grbitPlr) && !app.state.rgplr[lpth->iplr()].fInclude) {
                app.state.rgplr[lpth->iplr()].fInclude = 1;
                app.state.rgplr[lpth->iplr()].det = detSome;
            }
        }
    }
    return;
}

void SetVisPFFleets(APPSESSION &app, std::int16_t iPlr) {
    PLANET  *lpplMac;
    POINT16  pt;
    std::int16_t  pctCloak;
    std::int16_t  dy;
    FLEET   *lpfl2;
    std::int32_t  d2;
    PLANET  *lppl;
    std::int16_t  j;
    FLEET   *lpfl;
    THING   *lpth;
    std::int32_t  lRadius2;
    std::int16_t  ifl;
    THING   *lpthMac;
    std::int16_t  iRadius;
    std::int16_t  dx;
    std::uint16_t grbitPlr;
    std::int32_t  lRadPlanet2;
    std::int16_t  iRadPlanet;
    std::int16_t  iSteal;
    std::int16_t  pctDetect;
    std::int32_t  l;
    std::int32_t  lVis2;

    grbitPlr = iPlr == iplrNone ? 0 : 1 << iPlr;
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (!lpfl->fDead) {
            if (lpfl->iPlayer != iPlr) {
                if (!lpfl->fInclude && lpfl->idPlanet != idPlanetDeepSpace && app.state.lpPlanets[lpfl->idPlanet].iPlayer == iPlr) {
                    MarkFleet(app, lpfl, detSome);
                }
            } else {
                if (lpfl->fBombed && lpfl->idPlanet != idPlanetDeepSpace) {
                    MarkPlanet(app, app.state.lpPlanets + lpfl->idPlanet, iPlr, detSome);
                }
                iRadius = GetCachedFleetScannerRange(app, lpfl, &iRadPlanet, &pctDetect, &iSteal);
                iRadius = 0 <= iRadius ? iRadius : 0;
                lRadius2 = static_cast<std::uint32_t>((iRadius * iRadius));
                lRadPlanet2 = static_cast<std::uint32_t>((iRadPlanet * iRadPlanet));
                pt = lpfl->pt;
                for (j = 0; j < app.state.cFleet; j++) {
                    lpfl2 = app.state.rglpfl[j];
                    if (!app.state.rglpfl[j])
                        break;
                    if (!lpfl2->fDead) {
                        if ((iSteal & 1) && pt.x == lpfl2->pt.x && pt.y == lpfl2->pt.y && (!lpfl2->fInclude || lpfl2->det < detMore)) {
                            MarkFleet(app, lpfl2, detMore);
                        }
                        if (!lpfl2->fInclude) {
                            dx = std::abs(pt.x - lpfl2->pt.x);
                            if (dx <= iRadius) {
                                dy = std::abs(pt.y - lpfl2->pt.y);
                                if (dy <= iRadius) {
                                    l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                                    if (static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy)) <= lRadius2 && (lpfl2->idPlanet == idPlanetDeepSpace || l <= lRadPlanet2)) {
                                        pctCloak = PctCloakFromLpfl(app, lpfl2);
                                        if (pctDetect != 100) {
                                            pctCloak = static_cast<std::int16_t>((pctCloak * pctDetect)) / 100;
                                        }
                                        if (pctCloak == 0) {
                                            MarkFleet(app, lpfl2, detSome);
                                        } else if (l <= static_cast<std::int32_t>((lRadius2 * static_cast<std::int16_t>((100 - pctCloak)))) / 100 * static_cast<std::int16_t>((100 - pctCloak)) / 100 &&
                                                   (lpfl2->idPlanet == idPlanetDeepSpace ||
                                                    l <= static_cast<std::int32_t>((lRadPlanet2 * static_cast<std::int16_t>((100 - pctCloak)))) / 100 * static_cast<std::int16_t>((100 - pctCloak)) / 100)) {
                                            MarkFleet(app, lpfl2, detSome);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                lpth = app.state.lpThings;
                lpthMac = app.state.lpThings + app.state.cThing;
                for (; lpth < lpthMac; lpth++) {
                    if (iPlr != iplrNone) {
                        switch (lpth->ith()) {
                        case ithMinefield:
                        case ithMineralPacket:
                        case ithMysteryTrader:
                        case ithWormhole:
                            if ((lpth->ith() != ithMinefield || !(lpth->thm().grbitPlrNow() & grbitPlr)) && (lpth->ith() != ithMysteryTrader || !lpth->tht().fInclude()) &&
                                (lpth->ith() != ithMineralPacket || !lpth->thp().fInclude()) && (lpth->ith() != ithWormhole || !lpth->thw().fInclude())) {
                                dx = std::abs(pt.x - lpth->pt().x());
                                dy = std::abs(pt.y - lpth->pt().y());
                                l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                                if (l <= lRadius2 || lpth->ith() == ithMinefield) {
                                    if (lpth->ith() == ithMineralPacket) {
                                        lpth->thp().SetFInclude(1);
                                    LThIncPlr:
                                        if (!app.state.rgplr[lpth->iplr()].fInclude) {
                                            app.state.rgplr[lpth->iplr()].fInclude = 1;
                                            app.state.rgplr[lpth->iplr()].det = detSome;
                                        }
                                    } else if (lpth->ith() == ithMysteryTrader) {
                                        lpth->tht().SetFInclude(1);

                                    } else if (lpth->ith() == ithWormhole) {
                                        if (!(lpth->thw().grbitPlr() & grbitPlr) && l > static_cast<std::int32_t>((lRadius2 >> 4)) && l > lRadPlanet2)
                                            continue;
                                        lpth->thw().SetGrbitPlr(lpth->thw().grbitPlr() | grbitPlr);
                                        lpth->thw().SetFInclude(1);

                                    } else {
                                        if (((lpth->thm().grbitPlr() & grbitPlr) && l <= lRadius2) || l <= lRadPlanet2 || l <= static_cast<std::int32_t>((lRadius2 >> 4)) ||
                                            l <= lpth->thm().cMines()) {
                                            lpth->thm().SetGrbitPlr(lpth->thm().grbitPlr() | grbitPlr);
                                            lpth->thm().SetGrbitPlrNow(lpth->thm().grbitPlrNow() | grbitPlr);
                                            goto LThIncPlr;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if ((iSteal & 2) && lpfl->idPlanet != idPlanetDeepSpace) {
                    MarkPlanet(app, app.state.lpPlanets + lpfl->idPlanet, iPlr, detMore);
                }
                if (iRadPlanet > 0) {
                    iRadius = iRadPlanet;
                    lRadius2 = static_cast<std::uint32_t>((iRadius * iRadius));
                    pt = lpfl->pt;
                    lppl = app.state.lpPlanets;
                    lpplMac = app.state.lpPlanets + app.state.cPlanet;
                    for (; lppl < lpplMac; lppl++) {
                        if (!lppl->fInclude || lppl->det < detSome) {
                            dx = std::abs(app.state.rgptPlan[lppl->id].x - pt.x);
                            if (dx <= iRadius) {
                                dy = std::abs(app.state.rgptPlan[lppl->id].y - pt.y);
                                if (dy <= iRadius) {
                                    d2 = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                                    if (static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy)) <= lRadius2) {
                                        if (!lppl->fStarbase || lppl->iPlayer == iplrNone)
                                            goto LMark101;

                                        lVis2 = app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].lVisible();
                                        if (lVis2 < 10000 && d2 > static_cast<std::int32_t>((static_cast<std::int64_t>(lRadius2) * lVis2 / 10000))) {
                                            MarkPlanet(app, lppl, iPlr, detObscure);
                                            continue;
                                        }

                                    LMark101:
                                        MarkPlanet(app, lppl, iPlr, detSome);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return;
}

void SetVisPFPlanets(APPSESSION &app, std::int16_t iPlr) {
    std::int32_t  lRadPlanet2;
    std::int16_t  iRadPlanet;
    PLANET  *lpplMac;
    POINT16  pt;
    std::int16_t  pctCloak;
    PLANET  *lppl2;
    std::int16_t  dy;
    FLEET   *lpfl2;
    std::int32_t  d2;
    PLANET  *lppl;
    std::int16_t  j;
    THING   *lpth;
    std::int32_t  lRadius2;
    std::int16_t  i;
    THING   *lpthMac;
    std::int16_t  iRadius;
    std::int16_t  fStargateView;
    std::int16_t  dx;
    std::int32_t  l;
    PLANET  *lpplMac2;
    std::uint16_t grbitPlr;
    std::int16_t  rgStargateRange[16];
    std::int32_t  lVis2;

    grbitPlr = iPlr == iplrNone ? 0 : 1 << iPlr;
    fStargateView = 0;
    if (RaceQueryIO().pfnStat(&app.state.rgplr[iPlr], rsMajorAdv) == raStargate) {
        for (i = 0; i < 10; i++) {
            rgStargateRange[i] = 0;
            if (!app.state.rglpshdefSB[iPlr][i].fFree) {
                rgStargateRange[i] = StargateRangeFromLppl(app, nullptr, iPlr, i);
                if (rgStargateRange[i] > 0) {
                    fStargateView = 1;
                }
            }
        }
    }
    if (iPlr != iplrNone) {
        UpdateProgressGauge(app, progressStep1);
    }
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iPlr) {
            iRadius = GetPlanetScannerRange(app, lppl, &iRadPlanet);
            lRadius2 = static_cast<std::uint32_t>((iRadius * iRadius));
            lRadPlanet2 = static_cast<std::uint32_t>((iRadPlanet * iRadPlanet));
            pt = app.state.rgptPlan[lppl->id];
            for (j = 0; j < app.state.cFleet; j++) {
                lpfl2 = app.state.rglpfl[j];
                if (!app.state.rglpfl[j])
                    break;
                if (!lpfl2->fInclude && !lpfl2->fDead) {
                    dx = std::abs(pt.x - lpfl2->pt.x);
                    if (dx <= iRadius) {
                        dy = std::abs(pt.y - lpfl2->pt.y);
                        if (dy <= iRadius) {
                            l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                            if (static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy)) <= lRadius2 && (lpfl2->idPlanet == idPlanetDeepSpace || l <= lRadPlanet2)) {
                                pctCloak = PctCloakFromLpfl(app, lpfl2);
                                if (pctCloak == 0) {
                                    MarkFleet(app, lpfl2, detSome);
                                } else if (l <= static_cast<std::int32_t>((lRadius2 * static_cast<std::int16_t>((100 - pctCloak)))) / 100 * static_cast<std::int16_t>((100 - pctCloak)) / 100 &&
                                           (lpfl2->idPlanet == idPlanetDeepSpace ||
                                            l <= static_cast<std::int32_t>((lRadPlanet2 * static_cast<std::int16_t>((100 - pctCloak)))) / 100 * static_cast<std::int16_t>((100 - pctCloak)) / 100)) {
                                    MarkFleet(app, lpfl2, detSome);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (iPlr != iplrNone) {
        UpdateProgressGauge(app, progressStep1);
    }
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iPlr) {
            iRadius = GetPlanetScannerRange(app, lppl, &iRadPlanet);
            lRadius2 = static_cast<std::uint32_t>((iRadius * iRadius));
            lRadPlanet2 = static_cast<std::uint32_t>((iRadPlanet * iRadPlanet));
            pt = app.state.rgptPlan[lppl->id];
            lpth = app.state.lpThings;
            lpthMac = app.state.lpThings + app.state.cThing;
            for (; lpth < lpthMac; lpth++) {
                if (iPlr != iplrNone) {
                    switch (lpth->ith()) {
                    case ithMinefield:
                    case ithMineralPacket:
                    case ithMysteryTrader:
                    case ithWormhole:
                        if ((lpth->ith() != ithMinefield || !(lpth->thm().grbitPlrNow() & grbitPlr)) && (lpth->ith() != ithMysteryTrader || !lpth->tht().fInclude()) &&
                            (lpth->ith() != ithMineralPacket || !lpth->thp().fInclude()) && (lpth->ith() != ithWormhole || !lpth->thw().fInclude())) {
                            dx = std::abs(pt.x - lpth->pt().x());
                            if (dx <= iRadius) {
                                dy = std::abs(pt.y - lpth->pt().y());
                                if (dy <= iRadius) {
                                    l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                                    if (static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy)) <= lRadius2) {
                                        if (lpth->ith() == ithMineralPacket) {
                                            lpth->thp().SetFInclude(1);
                                        LThIncPlr2:
                                            if (!app.state.rgplr[lpth->iplr()].fInclude) {
                                                app.state.rgplr[lpth->iplr()].fInclude = 1;
                                                app.state.rgplr[lpth->iplr()].det = detSome;
                                            }
                                        } else if (lpth->ith() == ithMysteryTrader) {
                                            lpth->tht().SetFInclude(1);

                                        } else if (lpth->ith() == ithWormhole) {
                                            if (!(lpth->thw().grbitPlr() & grbitPlr) && l > static_cast<std::int32_t>((lRadius2 >> 4)) && l > lRadPlanet2)
                                                continue;
                                            lpth->thw().SetGrbitPlr(lpth->thw().grbitPlr() | grbitPlr);
                                            lpth->thw().SetFInclude(1);

                                        } else {
                                            if ((lpth->thm().grbitPlr() & grbitPlr) || l <= lRadPlanet2 || l <= static_cast<std::int32_t>((lRadius2 >> 4))) {
                                                lpth->thm().SetGrbitPlr(lpth->thm().grbitPlr() | grbitPlr);
                                                lpth->thm().SetGrbitPlrNow(lpth->thm().grbitPlrNow() | grbitPlr);
                                                goto LThIncPlr2;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (iPlr != iplrNone) {
        UpdateProgressGauge(app, progressStep1);
    }
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iPlr) {
            iRadius = GetPlanetScannerRange(app, lppl, &iRadPlanet);
            lRadius2 = static_cast<std::uint32_t>((iRadius * iRadius));
            lRadPlanet2 = static_cast<std::uint32_t>((iRadPlanet * iRadPlanet));
            pt = app.state.rgptPlan[lppl->id];
            if (fStargateView && lppl->fStarbase && rgStargateRange[lppl->isb] > 0) {
                iRadius = rgStargateRange[lppl->isb];
                lRadius2 = static_cast<std::uint32_t>((iRadius * iRadius));
                lppl2 = app.state.lpPlanets;
                lpplMac2 = app.state.lpPlanets + app.state.cPlanet;
                for (; lppl2 < lpplMac2; lppl2++) {
                    if ((!lppl2->fInclude || lppl2->det < detSome) && lppl2->fStarbase && StargateRangeFromLppl(app, lppl2, 0, 0) != 0) {
                        if (iRadius >= 10000)
                            goto LMarkStargate;

                        dx = std::abs(app.state.rgptPlan[lppl2->id].x - pt.x);
                        if (dx > iRadius)
                            continue;
                        dy = std::abs(app.state.rgptPlan[lppl2->id].y - pt.y);
                        if (dy > iRadius)
                            continue;
                        d2 = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                        if (static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy)) > lRadius2)
                            continue;
                        lVis2 = app.state.rglpshdefSB[lppl2->iPlayer][lppl2->isb].lVisible();
                        if (lVis2 < 10000 && d2 > static_cast<std::int32_t>((static_cast<std::int64_t>(lRadius2) * lVis2 / 10000)))
                            continue;

                    LMarkStargate:
                        MarkPlanet(app, lppl2, iPlr, detSome);
                    }
                }
            }
        }
    }
    if (iPlr != iplrNone) {
        UpdateProgressGauge(app, progressStep1);
    }
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iPlr) {
            iRadius = GetPlanetScannerRange(app, lppl, &iRadPlanet);
            lRadius2 = static_cast<std::uint32_t>((iRadius * iRadius));
            lRadPlanet2 = static_cast<std::uint32_t>((iRadPlanet * iRadPlanet));
            pt = app.state.rgptPlan[lppl->id];
            if (iRadPlanet > 0) {
                iRadius = iRadPlanet;
                lRadius2 = lRadPlanet2;
                lppl2 = app.state.lpPlanets;
                lpplMac2 = app.state.lpPlanets + app.state.cPlanet;
                for (; lppl2 < lpplMac2; lppl2++) {
                    if (!lppl2->fInclude || lppl2->det < detSome) {
                        dx = std::abs(app.state.rgptPlan[lppl2->id].x - pt.x);
                        if (dx <= iRadius) {
                            dy = std::abs(app.state.rgptPlan[lppl2->id].y - pt.y);
                            if (dy <= iRadius) {
                                d2 = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                                if (static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy)) <= lRadius2) {
                                    if (!lppl2->fStarbase || lppl2->iPlayer == iplrNone)
                                        goto LMark102;

                                    lVis2 = app.state.rglpshdefSB[lppl2->iPlayer][lppl2->isb].lVisible();
                                    if (lVis2 < 10000 && d2 > static_cast<std::int32_t>((static_cast<std::int64_t>(lRadius2) * lVis2 / 10000))) {
                                        MarkPlanet(app, lppl2, iPlr, detObscure);
                                        continue;
                                    }

                                LMark102:
                                    MarkPlanet(app, lppl2, iPlr, detSome);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return;
}

void SetVisPFThings(APPSESSION &app, std::int16_t iPlr) {
    POINT16  pt;
    std::int16_t  pctCloak;
    std::int16_t  dy;
    FLEET   *lpfl2;
    std::int32_t  d2;
    std::int16_t  j;
    THING   *lpth;
    std::int32_t  lRadius2;
    THING   *lpthMac;
    std::int16_t  iRadius;
    std::int16_t  dx;
    std::uint16_t grbitPlr;
    PLANET  *lppl2;
    THING   *lpthMac2;
    THING   *lpth2;
    std::int32_t  l;
    PLANET  *lpplMac2;
    std::int32_t  lVis2;

    grbitPlr = iPlr == iplrNone ? 0 : 1 << iPlr;
    if (RaceQueryIO().pfnStat(&app.state.rgplr[iPlr], rsMajorAdv) == raMassAccel) {
        lpth = app.state.lpThings;
        lpthMac = app.state.lpThings + app.state.cThing;
        for (; lpth < lpthMac; lpth++) {
            if (lpth->ith() == ithMineralPacket && lpth->iplr() == iPlr && lpth->thp().iWarp() != 0) {
                lpth->thp().SetFInclude(1);
                iRadius = lpth->thp().iWarp() + 4;
                iRadius *= iRadius;
                lRadius2 = static_cast<std::uint32_t>((iRadius * iRadius));
                pt = lpth->pt();
                for (j = 0; j < app.state.cFleet; j++) {
                    lpfl2 = app.state.rglpfl[j];
                    if (!app.state.rglpfl[j])
                        break;
                    if (!lpfl2->fInclude && !lpfl2->fDead) {
                        dx = std::abs(pt.x - lpfl2->pt.x);
                        if (dx <= iRadius) {
                            dy = std::abs(pt.y - lpfl2->pt.y);
                            if (dy <= iRadius) {
                                l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                                if (static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy)) <= lRadius2) {
                                    pctCloak = PctCloakFromLpfl(app, lpfl2);
                                    if (pctCloak == 0) {
                                        MarkFleet(app, lpfl2, detSome);
                                    } else if (l <= static_cast<std::int32_t>((lRadius2 * static_cast<std::int16_t>((100 - pctCloak)))) / 100 * static_cast<std::int16_t>((100 - pctCloak)) / 100) {
                                        MarkFleet(app, lpfl2, detSome);
                                    }
                                }
                            }
                        }
                    }
                }
                lpth2 = app.state.lpThings;
                lpthMac2 = app.state.lpThings + app.state.cThing;
                for (; lpth2 < lpthMac2; lpth2++) {
                    if (iPlr != iplrNone) {
                        switch (lpth2->ith()) {
                        case ithMinefield:
                        case ithMineralPacket:
                        case ithMysteryTrader:
                        case ithWormhole:
                            if ((lpth2->ith() != ithMinefield || !(lpth2->thm().grbitPlrNow() & grbitPlr)) &&
                                (lpth2->ith() != ithMysteryTrader || !lpth2->tht().fInclude()) && (lpth2->ith() != ithMineralPacket || !lpth2->thp().fInclude()) &&
                                (lpth2->ith() != ithWormhole || !lpth2->thw().fInclude())) {
                                dx = std::abs(pt.x - lpth2->pt().x());
                                if (dx <= iRadius) {
                                    dy = std::abs(pt.y - lpth2->pt().y());
                                    if (dy <= iRadius) {
                                        l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                                        if (static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy)) <= lRadius2) {
                                            if (lpth2->ith() == ithMineralPacket) {
                                                lpth2->thp().SetFInclude(1);
                                            LThIncPlr3:
                                                if (!app.state.rgplr[lpth2->iplr()].fInclude) {
                                                    app.state.rgplr[lpth2->iplr()].fInclude = 1;
                                                    app.state.rgplr[lpth2->iplr()].det = detSome;
                                                }
                                            } else if (lpth2->ith() == ithMysteryTrader) {
                                                lpth2->tht().SetFInclude(1);

                                            } else if (lpth2->ith() == ithWormhole) {
                                                if (!(lpth2->thw().grbitPlr() & grbitPlr) && l > lRadius2)
                                                    continue;
                                                lpth2->thw().SetGrbitPlr(lpth2->thw().grbitPlr() | grbitPlr);
                                                lpth2->thw().SetFInclude(1);

                                            } else {
                                                lpth2->thm().SetGrbitPlr(lpth2->thm().grbitPlr() | grbitPlr);
                                                lpth2->thm().SetGrbitPlrNow(lpth2->thm().grbitPlrNow() | grbitPlr);
                                                goto LThIncPlr3;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                lppl2 = app.state.lpPlanets;
                lpplMac2 = app.state.lpPlanets + app.state.cPlanet;
                for (; lppl2 < lpplMac2; lppl2++) {
                    if (!lppl2->fInclude || lppl2->det < detSome) {
                        dx = std::abs(app.state.rgptPlan[lppl2->id].x - pt.x);
                        if (dx <= iRadius) {
                            dy = std::abs(app.state.rgptPlan[lppl2->id].y - pt.y);
                            if (dy <= iRadius) {
                                d2 = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                                if (static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy)) <= lRadius2) {
                                    if (!lppl2->fStarbase || lppl2->iPlayer == iplrNone)
                                        goto LMark103;

                                    lVis2 = app.state.rglpshdefSB[lppl2->iPlayer][lppl2->isb].lVisible();
                                    if (lVis2 < 10000 && d2 > static_cast<std::int32_t>((static_cast<std::int64_t>(lRadius2) * lVis2 / 10000))) {
                                        MarkPlanet(app, lppl2, iPlr, detObscure);
                                        continue;
                                    }

                                LMark103:
                                    MarkPlanet(app, lppl2, iPlr, detSome);
                                }
                            }
                        }
                    }
                }
            }
        }
    } else if (RaceQueryIO().pfnStat(&app.state.rgplr[iPlr], rsMajorAdv) == raMines) {
        lpth = app.state.lpThings;
        lpthMac = app.state.lpThings + app.state.cThing;
        for (; lpth < lpthMac; lpth++) {
            if (lpth->ith() == ithMinefield && lpth->iplr() == iPlr) {
                lRadius2 = lpth->thm().cMines();
                pt = lpth->pt();
                for (j = 0; j < app.state.cFleet; j++) {
                    lpfl2 = app.state.rglpfl[j];
                    if (!app.state.rglpfl[j])
                        break;
                    if (!lpfl2->fInclude && !lpfl2->fDead && lpfl2->idPlanet == idPlanetDeepSpace) {
                        dx = std::abs(pt.x - lpfl2->pt.x);
                        if (dx <= lRadius2) {
                            dy = std::abs(pt.y - lpfl2->pt.y);
                            if (dy <= lRadius2) {
                                l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                                if (static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy)) <= lRadius2) {
                                    pctCloak = PctCloakFromLpfl(app, lpfl2);
                                    if (pctCloak == 0 || RandomIO().pfnRandom(app, 100) >= pctCloak) {
                                        MarkFleet(app, lpfl2, detSome);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return;
}

void SetVisPFFinish(APPSESSION &app, std::int16_t iPlr) {
    std::int16_t detMajor;
    std::int16_t j;
    std::int16_t i;

    detMajor = RaceQueryIO().pfnStat(&app.state.rgplr[iPlr], rsMajorAdv) == raAttack ? 7 : 3;
    for (i = 0; i < app.state.game.cPlayer; i++) {
        if (i != iPlr) {
            app.state.rgplr[i].cShDef = 0;
            for (j = 0; j < 16; j++) {
                if (1 << iPlr & app.state.rglpshdef[i][j].grbitPlr) {
                    app.state.rglpshdef[i][j].fInclude = 1;
                    app.state.rglpshdef[i][j].det = detAll;
                    goto LFinShdef;
                } else {
                    if (!app.state.rglpshdef[i][j].fInclude)
                        continue;
                    app.state.rglpshdef[i][j].det = detMajor;
                }
            LFinShdef:
                app.state.rgplr[i].det = detSome;
                app.state.rgplr[i].fInclude = 1;
                app.state.rgplr[i].cShDef++;
            }
            app.state.rgplr[i].cshdefSB = 0;
            for (j = 0; j < 10; j++) {
                if (1 << iPlr & app.state.rglpshdefSB[i][j].grbitPlr) {
                    app.state.rglpshdefSB[i][j].fInclude = 1;
                    app.state.rglpshdefSB[i][j].det = detAll;
                    goto LFinShdefSB;
                } else {
                    if (!app.state.rglpshdefSB[i][j].fInclude)
                        continue;
                    app.state.rglpshdefSB[i][j].det = detMajor;
                }
            LFinShdefSB:
                app.state.rgplr[i].fInclude = 1;
                app.state.rgplr[i].det = detSome;
                app.state.rgplr[i].cshdefSB++;
            }
        }
    }
    return;
}
