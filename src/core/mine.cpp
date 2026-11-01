#include "common.h"
#include "appstate.h"
#include "colony_io.h"
#include "race_query_io.h"
#include "random_io.h"

void GetMineFieldCounts(APPSESSION &app, std::uint16_t id, std::int16_t *pithm, std::int16_t *pcthm) {
    std::int16_t cthTotal;
    std::int16_t ithFound;
    THING  *lpth;
    THING  *lpthMac;

    ithFound = 0;
    cthTotal = 0;
    lpth = app.state.lpThings;
    lpthMac = app.state.lpThings + app.state.cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith() == ithMinefield && lpth->iplr() == app.state.idPlayer) {
            cthTotal++;
            if (lpth->idFull() == id) {
                ithFound = cthTotal;
            }
        }
    }
    *pithm = ithFound;
    *pcthm = cthTotal;
    return;
}

std::int16_t FOtherStuffAtScanSel(APPSESSION &app) {
    std::int16_t c;
    std::int16_t i;
    THING  *lpth;
    FLEET  *lpfl;
    THING  *lpthMac;

    if (app.state.sel.scan.idpl != idplNone && (app.state.sel.scan.ifl != iflNone || app.state.sel.scan.ith != ithNone)) {
        return 1;
    }
    if (app.state.sel.scan.ifl != iflNone) {
        if (app.state.sel.scan.ith != ithNone) {
            return 1;
        }
        c = 1;
        for (i = 0; i < app.state.cFleet; i++) {
            lpfl = app.state.rglpfl[i];
            if (!app.state.rglpfl[i])
                break;
            if (lpfl->pt.x == app.state.sel.scan.pt.x && lpfl->pt.y == app.state.sel.scan.pt.y && c-- == 0) {
                return 1;
            }
        }
    }
    if (app.state.sel.scan.ith != ithNone) {
        c = 1;
        lpth = app.state.lpThings;
        lpthMac = app.state.lpThings + app.state.cThing;
        for (; lpth < lpthMac; lpth++) {
            if (lpth->pt().x() == app.state.sel.scan.pt.x && lpth->pt().y() == app.state.sel.scan.pt.y && c-- == 0) {
                return 1;
            }
        }
    }
    return 0;
}

void EstMineralsMined(APPSESSION &app, PLANET *lppl, std::int32_t *plQuan, std::int32_t cMines, std::int16_t fApply) {
    std::int32_t lQuanRem;
    std::int32_t lQuanAct;
    std::int16_t i;
    std::int32_t lQuan;
    std::int16_t fMacintosh;
    std::int16_t fRemote;
    std::int32_t lMine;
    std::int32_t lMineEff;
    std::int32_t lConc;
    std::int32_t lLeft;
    std::int32_t lLevel;
    std::int32_t lLength;
    std::int32_t rglQuan[3];
    std::int16_t ifl;
    FLEET  *lpfl;

    fRemote = cMines != -1;
    fMacintosh = lppl->iPlayer != iplrNone && RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv) == raMacintosh;
    if (cMines == -1) {
        if (lppl->iPlayer == iplrNone || lppl->rgwtMin[3] == 0) {
            for (i = 0; i < 3; i++) {
                plQuan[i] = -1;
            }
            return;
        }
        lMine = ColonyIO().pfnMinesOperating(app, lppl);
        if (fMacintosh) {
            lMineEff = 10;
        } else {
            lMineEff = RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMineProd);
        }
    } else {
        lMine = cMines;
        lMineEff = 10;
    }
    for (i = 0; i <= 2; i++) {
        lConc = static_cast<std::uint32_t>(lppl->rgMinConc[i]);
        if (lConc < 30 && lppl->fHomeworld && (!fRemote || fMacintosh)) {
            lConc = 30;
        }
        lQuanAct = static_cast<std::uint32_t>((lMine * lConc));
        if (!fRemote) {
            lQuan = static_cast<std::int32_t>((lQuanAct * lMineEff)) / 10;
        } else {
            lQuan = lQuanAct;
        }
        lQuanRem = static_cast<std::int32_t>((lQuan % 100));
        lQuan = static_cast<std::int32_t>((lQuan / 100));
        if (lQuanRem != 0 && app.state.gd.fGeneratingTurn && RandomIO().pfnRandom(app, 100) < static_cast<std::int16_t>(LOWORD(lQuanRem))) {
            lQuan++;
        }
        plQuan[i] = lQuan;
        if (fApply) {
            lppl->rgwtMin[i] += lQuan;
            lQuanAct = static_cast<std::int32_t>((lQuanAct / 100));
            while (lQuanAct > 0 && lppl->rgMinConc[i] > 1) {
                lLevel = static_cast<std::uint32_t>(lppl->rgpctMinLevel[i]);
                lConc = static_cast<std::uint32_t>(lppl->rgMinConc[i]);
                if (lLevel == 0) {
                    lLevel = 256;
                }
                if (lConc > 100) {
                    lConc = 100;
                } else if (lConc < 5) {
                    lConc = 10;
                } else if (lConc < 25) {
                    lConc = 25;
                }
                lLeft = static_cast<std::int32_t>((static_cast<std::int32_t>((lLevel * 12500)) / 256 / lConc));
                if (lLeft <= lQuanAct) {
                    lQuanAct -= lLeft;
                    lppl->rgMinConc[i]--;
                    lppl->rgpctMinLevel[i] = 0;
                } else {
                    lLength = static_cast<std::int32_t>((12500 / lConc));
                    lLeft = static_cast<std::int32_t>((static_cast<std::int32_t>(((lLeft - lQuanAct) * 0x100)) / lLength));
                    if (lLeft < 1) {
                        lLeft = 1;
                    }
                    if (lLeft >= lLevel) {
                        lLeft = lLevel - 1;
                    }
                    lppl->rgpctMinLevel[i] = lLeft;
                    if (lLeft == 0) {
                        lppl->rgMinConc[i]--;
                    }
                    break;
                }
            }
        }
    }
    if (fMacintosh && !fRemote) {
        for (ifl = 0; ifl < app.state.cFleet; ifl++) {
            lpfl = app.state.rglpfl[ifl];
            if (!app.state.rglpfl[ifl])
                break;
            if (lpfl->idPlanet == lppl->id && lpfl->iPlayer == lppl->iPlayer && !lpfl->fDead && lpfl->cord <= 1 &&
                lpfl->lpplord->rgord()[0].grTask == grTaskMine) {
                cMines = ColonyIO().pfnFleetMining(app, lpfl);
                if (cMines > 0) {
                    EstMineralsMined(app, lppl, rglQuan, cMines, fApply);
                    for (i = 0; i < 3; i++) {
                        plQuan[i] += rglQuan[i];
                    }
                    if (fApply) {
                        lpfl->fHereAllTurn = 0;
                    }
                }
            }
        }
    }
    return;
}
