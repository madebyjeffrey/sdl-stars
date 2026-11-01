#ifndef STARS_APPSTATE_H
#define STARS_APPSTATE_H
#include "common.h"
#include <cstddef>
#include "filesession.h"

// Original debug names remain owned fields; callers receive the owner explicitly.
// Packed runtime types retain the stronger alignment of their former globals.
struct APPGAMESTATE {
    APPGAMESTATE() noexcept;
    alignas(std::max_align_t) std::int32_t lFileSeed1 = 0;
    alignas(std::max_align_t) std::int32_t lFileSeed2 = 0;
    alignas(std::max_align_t) BTLPLAN *rglpbtlplan[16] = {};
    alignas(std::max_align_t) COLDROP *lpcd = 0;
    alignas(std::max_align_t) FLEET  **rglpfl = 0;
    alignas(std::max_align_t) GAME     game = {};
    alignas(std::max_align_t) GDATA    gd = {};
    alignas(std::max_align_t) HB      *rglphb[12] = {};
    alignas(std::max_align_t) HDR      hdrCur = {};
    alignas(std::max_align_t) HDR      hdrPrev = {};
    alignas(std::max_align_t) INI      ini = {};
    alignas(std::max_align_t) LOGXFER  lx = {};
    alignas(std::max_align_t) LOGXFERF lxf = {};
    alignas(std::max_align_t) MSGPLR  *vlpmsgplrIn = 0;
    alignas(std::max_align_t) MSGPLR  *vlpmsgplrOut = 0;
    alignas(std::max_align_t) PLANET **vrglpplAi = 0;
    alignas(std::max_align_t) PLANET  *lpPlanets = 0;
    alignas(std::max_align_t) PLAYER   rgplr[16] = {};
    alignas(std::max_align_t) PLAYER   vplr = {};
    alignas(std::max_align_t) PLPROD  *lpplProdGlob = 0;
    alignas(std::max_align_t) POINT16  rgptPlan[999] = {};
    alignas(std::max_align_t) PROD    *pProdGlob = 0;
    alignas(std::max_align_t) RPT      vrptBattle = {};
    alignas(std::max_align_t) RPT vrptEFleet = {};
    alignas(std::max_align_t) RPT vrptFleet = {};
    alignas(std::max_align_t) RPT vrptPlanet = {};
    alignas(std::max_align_t) SCOREX   *rgsxPlr[16] = {};
    alignas(std::max_align_t) SCOREX   *vlprgScoreX = 0;
    alignas(std::max_align_t) SEL       sel = {};
    alignas(std::max_align_t) SHDEF    *rglpshdefSB[16] = {};
    alignas(std::max_align_t) SHDEF    *rglpshdef[16] = {};
    alignas(std::max_align_t) SHDEF     rgshdef[16] = {};
    alignas(std::max_align_t) SHDEF     shdefBuild = {};
    alignas(std::max_align_t) ScanZoom  iScanZoom = zoom100;
    alignas(std::max_align_t) THING    *lpThings = 0;
    alignas(std::max_align_t) THING    *lpthBattle = 0;
    alignas(std::max_align_t) TOK      *vrgtok = 0;
    alignas(std::max_align_t) TUTOR     tutor = {};
    alignas(std::max_align_t) XFER     *pxfer = 0;
    alignas(std::max_align_t) XFERFULL *lpxf = 0;
    alignas(std::max_align_t) ZIPPRODQ  vrgZipProd[5] = {};
    alignas(std::max_align_t) const char     *MPCTD = "m%d";
    alignas(std::max_align_t) const char     *PCTD = "%d";
    alignas(std::max_align_t) const char     *PCTDPCTPCT = "%d%%";
    alignas(std::max_align_t) const char     *PCTDXPCTDPCTPCT = "%d.%d%%";
    alignas(std::max_align_t) const char     *PCTLD = "%ld";
    alignas(std::max_align_t) char     *lpbDefMac = 0;
    alignas(std::max_align_t) char     *lpbDefUni = 0;
    alignas(std::max_align_t) const char     *mpdtsz[8] = {"xy", "x", "hst", "m", "h", "r", "log", "chk"};
    alignas(std::max_align_t) const char     *rgchcompstrlower = " aehilnorstbcdfgjkmpquvwxyz";
    alignas(std::max_align_t) const char     *rgszMineField[3] = {"Standard", "Heavy", "Speed Bump"};
    alignas(std::max_align_t) const char     *rgszMinerals[6] = {"Ironium", "Boranium", "Germanium", "Colonists", "Fuel", "Resources"};
    alignas(std::max_align_t) const char     *rgszPlanetAttr[3] = {"Gravity", "Temperature", "Radiation"};
    alignas(std::max_align_t) const char     *vrgszUnits[6] = {"kT", "kT", "kT", "00", "mg", "% "};
    alignas(std::max_align_t) char      iLastGet = -1;
    alignas(std::max_align_t) char      iLastMsgGet = -1;
    alignas(std::max_align_t) char      iLastStrGet = -1;
    alignas(std::max_align_t) char      rgbCur[1024] = "";
    alignas(std::max_align_t) char      rgchcomp[13] = "+-,!.?:;'*%$";
    alignas(std::max_align_t) char      szBackup[256] = "";
    alignas(std::max_align_t) char      szBase[256] = "";
    alignas(std::max_align_t) char      szFormatNumber[12] = "";
    alignas(std::max_align_t) char      szLastGet[19] = "";
    alignas(std::max_align_t) char      szLastMsgGet[256] = "";
    alignas(std::max_align_t) char      szLastStrGet[256] = "";
    alignas(std::max_align_t) char      szMsgBuf[256] = "";
    alignas(std::max_align_t) char      szPassLast[16] = "";
    alignas(std::max_align_t) char      szRaceFile[16] = "";
    alignas(std::max_align_t) char      szRacePass[16] = "";
    alignas(std::max_align_t) char      szWork[360] = "";
    alignas(std::max_align_t) char      vszDefPass[17] = "";
    alignas(std::max_align_t) std::uint8_t  *lpMsg = 0;
    alignas(std::max_align_t) std::int16_t  *vrgiflMerge = 0;
    alignas(std::max_align_t) std::int16_t  *vrgiplrPopScore = 0;
    alignas(std::max_align_t) std::int16_t   bitTbl[8] = {1, 2, 4, 8, 16, 32, 64, 128};
    alignas(std::max_align_t) std::int16_t   cColDrop = 0;
    alignas(std::max_align_t) std::int16_t   cFleet = 0;
    alignas(std::max_align_t) std::int16_t   cMsg = 0;
    alignas(std::max_align_t) std::int16_t   cPlanet = 0;
    alignas(std::max_align_t) std::int16_t   cProdGlob = 0;
    alignas(std::max_align_t) std::int16_t   cRandStack = 0;
    alignas(std::max_align_t) std::int16_t   cThing = 0;
    alignas(std::max_align_t) std::int16_t   cThingAlloc = 0;
    alignas(std::max_align_t) std::int16_t   cXferFull = 0;
    alignas(std::max_align_t) std::int16_t   cbbitfMsg = 49;
    alignas(std::max_align_t) std::int16_t   dGal = 2000;
    alignas(std::max_align_t) std::int16_t   dGalInv = 4000;
    alignas(std::max_align_t) std::int16_t   dGalMinDist = 12;
    alignas(std::max_align_t) std::int16_t   dyArial8 = 0;
    alignas(std::max_align_t) std::int16_t   fAi = 0;
    alignas(std::max_align_t) std::int16_t   fFileErrSilent = 0;
    alignas(std::max_align_t) std::int16_t   fLogOff = 0;
    alignas(std::max_align_t) std::int16_t   fMarkedPlanets = 0;
    alignas(std::max_align_t) std::int16_t   fRCWReadOnly = 0;
    alignas(std::max_align_t) std::int16_t   fStarbaseDamaged = 0;
    alignas(std::max_align_t) std::int16_t   fStarbaseDied = 0;
    alignas(std::max_align_t) std::int16_t   fStarbaseMode = 0;
    alignas(std::max_align_t) std::int16_t   fValidLx = 0;
    alignas(std::max_align_t) std::int16_t   fValidLxf = 0;
    alignas(std::max_align_t) std::int16_t   fViewFilteredMsg = 0;
    alignas(std::max_align_t) std::int16_t   hf = -1;
    alignas(std::max_align_t) std::int16_t   iMsgCur = 0;
    alignas(std::max_align_t) std::int16_t   iMsgSendCur = 0;
    alignas(std::max_align_t) std::int16_t   idBattle = 0;
    alignas(std::max_align_t) std::int16_t   idPlayer = 1;
    alignas(std::max_align_t) std::int16_t   idsFileError = -1;
    alignas(std::max_align_t) std::int16_t   imemLogCur = 0;
    alignas(std::max_align_t) std::int16_t   imemLogPrev = -1;
    alignas(std::max_align_t) std::int16_t   imemMsgCur = 0;
    alignas(std::max_align_t) std::int16_t   rgcompstrlower[26] = {1, 77, 93, 109, 2, 125, 141, 3, 4, 157, 173, 5, 189, 6, 7, 205, 221, 8, 9, 10, 237, 253, 14, 30, 46, 62};
    alignas(std::max_align_t) std::int16_t   rgcsxPlr[16] = {};
    alignas(std::max_align_t) std::int16_t   rgidPlan[999] = {};
    alignas(std::max_align_t) std::int16_t   vcBackupDirs = 1;
    alignas(std::max_align_t) std::int16_t   vcflMerge = 0;
    alignas(std::max_align_t) std::int16_t   vclpplAi = 0;
    alignas(std::max_align_t) std::int16_t   vcmsgplrIn = 0;
    alignas(std::max_align_t) std::int16_t   vcmsgplrOut = 0;
    alignas(std::max_align_t) std::int16_t   vctok = 0;
    alignas(std::max_align_t) std::int16_t   vretExitValue = 0;
    alignas(std::max_align_t) std::int32_t  *vrgwtPopScore = 0;
    alignas(std::max_align_t) std::int32_t   lRandSeed1 = 17;
    alignas(std::max_align_t) std::int32_t   lRandSeed2 = 37;
    alignas(std::max_align_t) std::int32_t   lSaltCur = 0;
    alignas(std::max_align_t) std::int32_t   lSaltLast = 0;
    alignas(std::max_align_t) std::int32_t   rglPopMac[5] = {2500, 5000, 10000, 20000, 30000};
    alignas(std::max_align_t) std::int32_t   rglRandStack[4][2] = {};
    alignas(std::max_align_t) std::uint16_t *vlprgidFleet = 0;
    alignas(std::max_align_t) std::uint16_t *vlprgidPlanet = 0;
    alignas(std::max_align_t) std::uint16_t *vlpwtCargo = 0;
    alignas(std::max_align_t) std::uint16_t *vrgPlanResExtra = 0;
    alignas(std::max_align_t) std::uint16_t *vrgPlrLosses = 0;
    alignas(std::max_align_t) std::uint16_t  grfMissed = 0;
    alignas(std::max_align_t) std::uint16_t  mphtcbAlloc[12] = {63488, 4096, 4096, 4096, 8192, 63488, 65280, 17472, 4096, 6144, 2048, 65280};
    alignas(std::max_align_t) std::uint16_t  wVersFile = 0;
    alignas(std::max_align_t) std::uint32_t  ctickLast = 0;
    alignas(std::max_align_t) std::uint8_t  *lpLog = 0;
    alignas(std::max_align_t) std::uint8_t  *lpbBattleCur = 0;
    alignas(std::max_align_t) std::uint8_t  *lpbBattleLog = 0;
    alignas(std::max_align_t) std::uint8_t  *lpbBattleT = 0;
    alignas(std::max_align_t) std::uint8_t  *vAiMacRecycleSB = 0;
    alignas(std::max_align_t) std::uint8_t  *vlpMemStream = 0;
    alignas(std::max_align_t) std::uint8_t  *vlpbAiData = 0;
    alignas(std::max_align_t) std::uint8_t  *vlpbAiPlanet = 0;
    alignas(std::max_align_t) std::uint8_t   bitfMsgFiltered[49] = {};
    alignas(std::max_align_t) std::uint8_t   bitfMsgSent[49] = {};
    alignas(std::max_align_t) std::uint8_t   rgTechBattle[6] = {};
    alignas(std::max_align_t) std::uint8_t   rgTechTrader[13] = {};
    alignas(std::max_align_t) std::uint8_t   rgcbtlplan[16] = {};
    alignas(std::max_align_t) std::uint8_t   vrgAiArmadaPotency[4] = {};
    alignas(std::max_align_t) std::uint8_t   vrgAiCyberArmadaPotency[4] = {};
    alignas(std::max_align_t) std::uint8_t   vrgplrTypeNew[16] = {};
    alignas(std::max_align_t) std::int16_t   rgOut[16] = {};
    alignas(std::max_align_t) char     *lpchBatch = 0;
    alignas(std::max_align_t) char     *lpchBatchMac = 0;
    alignas(std::max_align_t) const char     *rgszZipOrder[4] = {"QuikLoad", "QuikDrop", "WaitLoad", "Clear"};
    alignas(std::max_align_t) const char     *szDblDash = "-- ";
    alignas(std::max_align_t) char      szCRLF[3] = "\r\n";
    alignas(std::max_align_t) ZIPORDER  vrgZip[4] = {};
};

struct APPUISTATE;

class APPSESSION {
public:
    // The derived desktop owner supplies its stable UI value; the host leaves it null.
    APPUISTATE *const lpui;
    APPGAMESTATE state;
    NATIVEFILESESSION files;
    NATIVEGAMEHEAPS heaps{*this};
    NATIVEFILESESSION *previous = nullptr;
    bool fFilesBound = false;
    ~APPSESSION() noexcept { Close(); }
    explicit APPSESSION(APPUISTATE *lpui = nullptr) noexcept : lpui(lpui) { BindFiles(); }
    APPSESSION(const APPSESSION &) = delete;
    APPSESSION &operator=(const APPSESSION &) = delete;
    void BindFiles() noexcept {
        if (!fFilesBound) {
            previous = PBindNativeFileSession(&files);
            fFilesBound = true;
        }
    }
    void Close() noexcept {
        files.Close();
        heaps.Close();
        if (fFilesBound) {
            PBindNativeFileSession(previous);
            fFilesBound = false;
        }
    }
    // The caller stops UI borrowers before resetting their game state.
    void Reset() noexcept { Close(); state = APPGAMESTATE{}; BindFiles(); }
};
namespace host { extern APPSESSION app; }

#endif
