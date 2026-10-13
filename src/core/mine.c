#include "common.h"

void GetMineFieldCounts(uint16_t id, int16_t *pithm, int16_t *pcthm) {
    int16_t cthTotal;
    int16_t ithFound;
    THING  *lpth;
    THING  *lpthMac;

    ithFound = 0;
    cthTotal = 0;
    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith == ithMinefield && lpth->iplr == idPlayer) {
            cthTotal++;
            if (lpth->idFull == id) {
                ithFound = cthTotal;
            }
        }
    }
    *pithm = ithFound;
    *pcthm = cthTotal;
    return;
}

int16_t FOtherStuffAtScanSel() {
    int16_t c;
    int16_t i;
    THING  *lpth;
    FLEET  *lpfl;
    THING  *lpthMac;

    if (sel.scan.idpl != idplNone && (sel.scan.ifl != iflNone || sel.scan.ith != ithNone)) {
        return TRUE;
    }
    if (sel.scan.ifl != iflNone) {
        if (sel.scan.ith != ithNone) {
            return TRUE;
        }
        c = 1;
        for (i = 0; i < cFleet; i++) {
            lpfl = rglpfl[i];
            if (!rglpfl[i])
                break;
            if (lpfl->pt.x == sel.scan.pt.x && lpfl->pt.y == sel.scan.pt.y && c-- == 0) {
                return TRUE;
            }
        }
    }
    if (sel.scan.ith != ithNone) {
        c = 1;
        lpth = lpThings;
        lpthMac = lpThings + cThing;
        for (; lpth < lpthMac; lpth++) {
            if (lpth->pt.x == sel.scan.pt.x && lpth->pt.y == sel.scan.pt.y && c-- == 0) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

void EstMineralsMined(PLANET *lppl, int32_t *plQuan, int32_t cMines, int16_t fApply) {
    int32_t lQuanRem;
    int32_t lQuanAct;
    int16_t i;
    int32_t lQuan;
    int16_t fMacintosh;
    int16_t fRemote;
    int32_t lMine;
    int32_t lMineEff;
    int32_t lConc;
    int32_t lLeft;
    int32_t lLevel;
    int32_t lLength;
    int32_t rglQuan[3];
    int16_t ifl;
    FLEET  *lpfl;

    fRemote = cMines != -1;
    fMacintosh = lppl->iPlayer != iplrNone && GetRaceStat(&rgplr[lppl->iPlayer], rsMajorAdv) == raMacintosh;
    if (cMines == -1) {
        if (lppl->iPlayer == iplrNone || lppl->rgwtMin[3] == 0) {
            for (i = 0; i < 3; i++) {
                plQuan[i] = -1;
            }
            return;
        }
        lMine = CMinesOperating(lppl);
        if (fMacintosh) {
            lMineEff = 10;
        } else {
            lMineEff = GetRaceStat(&rgplr[lppl->iPlayer], rsMineProd);
        }
    } else {
        lMine = cMines;
        lMineEff = 10;
    }
    for (i = 0; i <= 2; i++) {
        lConc = (uint32_t)lppl->rgMinConc[i];
        if (lConc < 30 && lppl->fHomeworld && (!fRemote || fMacintosh)) {
            lConc = 30;
        }
        lQuanAct = (uint32_t)(lMine * lConc);
        if (!fRemote) {
            lQuan = (int32_t)(lQuanAct * lMineEff) / 10;
        } else {
            lQuan = lQuanAct;
        }
        lQuanRem = (int32_t)(lQuan % 100);
        lQuan = (int32_t)(lQuan / 100);
        if (lQuanRem != 0 && gd.fGeneratingTurn && Random(100) < (int16_t)LOWORD(lQuanRem)) {
            lQuan++;
        }
        plQuan[i] = lQuan;
        if (fApply) {
            lppl->rgwtMin[i] += lQuan;
            lQuanAct = (int32_t)(lQuanAct / 100);
            while (lQuanAct > 0 && lppl->rgMinConc[i] > 1) {
                lLevel = (uint32_t)lppl->rgpctMinLevel[i];
                lConc = (uint32_t)lppl->rgMinConc[i];
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
                lLeft = (int32_t)((int32_t)(lLevel * 12500) / 256 / lConc);
                if (lLeft <= lQuanAct) {
                    lQuanAct -= lLeft;
                    lppl->rgMinConc[i]--;
                    lppl->rgpctMinLevel[i] = 0;
                } else {
                    lLength = (int32_t)(12500 / lConc);
                    lLeft = (int32_t)((int32_t)((lLeft - lQuanAct) * 0x100) / lLength);
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
        for (ifl = 0; ifl < cFleet; ifl++) {
            lpfl = rglpfl[ifl];
            if (!rglpfl[ifl])
                break;
            if (lpfl->idPlanet == lppl->id && lpfl->iPlayer == lppl->iPlayer && !lpfl->fDead && lpfl->cord <= 1 &&
                lpfl->lpplord->rgord[0].grTask == grTaskMine) {
                cMines = CMineFromLpfl(lpfl);
                if (cMines > 0) {
                    EstMineralsMined(lppl, rglQuan, cMines, fApply);
                    for (i = 0; i < 3; i++) {
                        plQuan[i] += rglQuan[i];
                    }
                    if (fApply) {
                        lpfl->fHereAllTurn = FALSE;
                    }
                }
            }
        }
    }
    return;
}
