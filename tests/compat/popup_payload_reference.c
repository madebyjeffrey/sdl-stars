// Independent C oracle: retain the frozen popup union's numeric carry.
#include "../reference/cpp-a/common.h"
static POPUPDATA pdReference;
static FLEET flReference;
static SHDEF shReference;
static ARMOR armorReference;
static char szReference[] = "popup";
void InitPopupReference(void) { memset(&pdReference, 0, sizeof(pdReference)); }
uint16_t WPopupReference(uint16_t op, int32_t value) {
    int16_t w = (int16_t)(uint16_t)value;
    switch (op) {
    case 0: pdReference.rgi[0] = value; break;
    case 1: pdReference.rgi[1] = value; break;
    case 2: pdReference.rgi[2] = value; break;
    case 3: pdReference.rgi[3] = value; break;
    case 4: pdReference.rgi[4] = value; break;
    case 5: pdReference.iPlrMin = w; break;
    case 6: pdReference.iPlanMax = w; break;
    case 7: pdReference.fFactory = w; break;
    case 8: pdReference.fToken = w; break;
    case 9: pdReference.grbit = (uint16_t)w; break;
    case 10: pdReference.idPlanet = w; break;
    case 11: pdReference.iPlanetVar = w; break;
    case 12: pdReference.iPlanVal = w; break;
    case 13: pdReference.iPlanMin = w; break;
    case 14: pdReference.iPlrVal = w; break;
    case 15: pdReference.iPlrMax = w; break;
    case 16: pdReference.idPlan = w; break;
    case 17: pdReference.cMax = w; break;
    case 18: pdReference.cCur = w; break;
    case 19: pdReference.cOperate = w; break;
    case 20: pdReference.dxOut = w; break;
    case 21: pdReference.iPlayer = w; break;
    case 22: pdReference.fRedDamage = w; break;
    case 23: pdReference.dxDamage = w; break;
    case 24: pdReference.fShowDamage = w; break;
    case 25: pdReference.fHideCounts = w; break;
    case 26: pdReference.fSummary = w; break;
    case 27: pdReference.itok = w; break;
    case 28: pdReference.lpfl = &flReference; break;
    case 29: pdReference.psz = szReference; break;
    case 30: pdReference.lpshdef = &shReference; break;
    case 31: pdReference.part.parmor = &armorReference; break;
    case 32: pdReference.part.hs.grhst = (uint16_t)value; break;
    case 33: { PART part; memset(&part, 0, sizeof(part)); part.parmor = &armorReference; pdReference.part = part; break; }
    }
    return pdReference.grbit;
}
