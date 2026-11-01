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

// The same frozen declaration independently observes native transfer aliases.
static XFER xfReference;
void InitXferReference(void) { memset(&xfReference, 0, sizeof(xfReference)); }
void ApplyXferReference(uint16_t op, int32_t value, void *pv, int32_t *out) {
    int16_t w = (int16_t)(uint16_t)value;
    switch (op) {
    case 0: xfReference.fl.id = w; break;
    case 1: xfReference.fl.iPlayer = w; break;
    case 2: xfReference.pl.id = w; break;
    case 3: xfReference.pl.iPlayer = w; break;
    case 4: xfReference.th.idFull = (uint16_t)value; break;
    case 5: xfReference.th.pt.x = w; break;
    case 6: xfReference.th.pt.y = w; break;
    case 7: xfReference.pl.rgwtMin[0] = value; break;
    case 8: xfReference.fl.rgcsh[0] = w; break;
    case 9: xfReference.fl.det = (uint16_t)value; break;
    case 10: xfReference.fl.rgwtMin[0] = value; break;
    case 11: xfReference.pl.rgEnvVar[0] = (int8_t)(uint8_t)value; break;
    case 12: xfReference.fl.lpplord = (PLORD *)pv; break;
    case 13: xfReference.pl.lpplprod = (PLPROD *)pv; break;
    case 14: {
        THING th; memset(&th, 0, sizeof(th));
        th.idFull = (uint16_t)value; th.pt.x = w; th.pt.y = (int16_t)(uint16_t)~(uint16_t)value;
        th.thp.idPlanet = (uint16_t)value; th.thp.iWarp = (uint16_t)value >> 10;
        th.thp.fMoved = (uint16_t)value >> 14; th.thp.fInclude = (uint16_t)value >> 15;
        th.thp.rgwtMin[0] = w; th.turn = (uint16_t)value;
        xfReference.th = th; break;
    }
    case 15: {
        FLEET fl; memset(&fl, 0, sizeof(fl));
        fl.id = w; fl.iPlayer = (int16_t)((uint32_t)value >> 16);
        fl.rgwtMin[0] = value; fl.rgcsh[0] = w; fl.lpplord = (PLORD *)pv;
        xfReference.fl = fl; break;
    }
    case 16: xfReference.id = w; break;
    case 17: xfReference.grobj = (GrobjClass)(uint16_t)value; break;
    }
    out[0]=xfReference.fl.id; out[1]=xfReference.fl.iPlayer; out[2]=xfReference.fl.det;
    out[3]=xfReference.th.idFull; out[4]=xfReference.th.pt.x; out[5]=xfReference.th.pt.y;
    out[6]=xfReference.fl.rgcsh[0]; out[7]=xfReference.fl.rgwtMin[0]; out[8]=xfReference.pl.rgwtMin[0];
    out[9]=xfReference.pl.rgEnvVar[0]; out[10]=xfReference.fl.lpplord==(PLORD *)pv;
    out[11]=xfReference.pl.lpplprod==(PLPROD *)pv; out[12]=xfReference.id; out[13]=xfReference.grobj;
}
