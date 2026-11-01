#ifndef STARS_DECOMPILED_STRUCTS_H
#define STARS_DECOMPILED_STRUCTS_H

class APPSESSION;

#include <stdint.h>
#include <cstddef>
#include <array>
#include <bit>
#include <span>
#include "taskviews.h"

#include "enums.h"
#include "native.h"
#include "thingviews.h"
#include "nativeobject.h"
#include <algorithm>

typedef struct _aipart          AIPART;
typedef struct _aistarbase      AISTARBASE;
typedef struct _aihist          AIHIST;
typedef struct _armor           ARMOR;
typedef struct _beam            BEAM;
typedef struct _bomb            BOMB;
typedef struct _btlplan         BTLPLAN;
typedef struct _coldrop         COLDROP;
typedef struct _compart         COMPART;
typedef struct _cyberinfo       CYBERINFO;
typedef struct _cyberinfotemp   CYBERINFOTEMP;
typedef struct _dv              DV;
typedef struct _engine          ENGINE;
typedef struct _fleetid         FLEETID;
typedef struct _framestuff      FRAMESTUFF;
typedef struct _game            GAME;
typedef struct _gdata           GDATA;
typedef struct _hb              HB;
typedef struct _hdr             HDR;
typedef struct _hs              HS;
typedef struct _hul             HUL;
typedef struct _huldef          HULDEF;
typedef struct _itemaction      ITEMACTION;
typedef struct _kill            KILL;
typedef struct _btlrec          BTLREC;
typedef struct _btlrec26        BTLREC26;
typedef struct _logxfer         LOGXFER;
typedef struct _logxferf        LOGXFERF;
typedef struct _lsb             LSB;
typedef struct _mdplr           MDPLR;
typedef struct _mines           MINES;
typedef struct _mining          MINING;
typedef struct _msgbig          MSGBIG;
typedef struct _msghdr          MSGHDR;
typedef struct _msgplr          MSGPLR;
typedef struct _msgturn         MSGTURN;
typedef struct _obj             OBJ;
typedef struct _part            PART;
typedef struct _pl              PL;
typedef struct _planet          PLANET;
typedef struct _planetary       PLANETARY;
typedef struct _planetminimal   PLANETMINIMAL;
typedef struct _planetsome      PLANETSOME;
typedef struct _fleet           FLEET;
typedef struct _fleetsome       FLEETSOME;
typedef struct _popupdata       POPUPDATA;
typedef struct _prod            PROD;
typedef struct PLPROD           PLPROD;
typedef struct _prodq1          PRODQ1;
typedef struct _btn             BTN;
typedef struct _btnt            BTNT;
typedef struct _drawcir         DRAWCIR;
typedef struct _rpt             RPT;
typedef struct _rtbof           RTBOF;
typedef struct _rtchgname       RTCHGNAME;
typedef struct _rtChgPlanetLong RTCHGPLANETLONG;
typedef struct _rtChgProdQ      RTCHGPRODQ;
typedef struct _rthisthdr       RTHISTHDR;
typedef struct _rtloghdr        RTLOGHDR;
typedef struct _rtlogthing      RTLOGTHING;
typedef struct _rtplanet        RTPLANET;
typedef struct _rtshdef         RTSHDEF;
typedef struct _rtchgshdef      RTCHGSHDEF;
typedef struct _rtfleet         RTFLEET;
typedef struct _rtfleetids      RTFLEETIDS;
typedef struct _rtplrmsg        RTPLRMSG;
typedef struct _rtresearch      RTRESEARCH;
typedef struct _rtshipint       RTSHIPINT;
typedef struct _rtshipint2      RTSHIPINT2;
typedef struct _rtxfer          RTXFER;
typedef struct _rtxferf         RTXFERF;
typedef struct _rtxferl         RTXFERL;
typedef struct _rtxferx         RTXFERX;
typedef struct _sbar            SBAR;
typedef struct _scan            SCAN;
typedef struct _scanner         SCANNER;
typedef struct _score           SCORE;
typedef struct _scorex          SCOREX;
typedef struct _selSome         SELSOME;
typedef struct _shdef           SHDEF;
typedef struct _shield          SHIELD;
typedef struct _special         SPECIAL;
typedef struct _specialsb       SPECIALSB;
typedef struct _starpack        STARPACK;
typedef struct _tasklaymines    TASKLAYMINES;
typedef struct _taskpatrol      TASKPATROL;
typedef struct _tasksell        TASKSELL;
typedef struct _taskxport       TASKXPORT;
typedef struct _order           ORDER;
typedef struct PLORD            PLORD;
typedef struct _rtwaypt         RTWAYPT;
typedef struct _terra           TERRA;
typedef struct _thmine          THMINE;
typedef struct _thpack          THPACK;
typedef struct _thtrader        THTRADER;
typedef struct _thworm          THWORM;
typedef struct _thing           THING;
typedef struct _sel             SEL;
typedef struct _tile            TILE;
typedef struct _timer           TIMER;
typedef struct _tok             TOK;
typedef struct _btldata         BTLDATA;
typedef struct _torp            TORP;
typedef struct _vers            VERS;
typedef struct _wn              WN;
typedef struct _ini             INI;
typedef struct _xfer            XFER;
typedef struct _xferfull        XFERFULL;
typedef struct _ziporder        ZIPORDER;
typedef struct _zipprodq1       ZIPPRODQ1;
typedef struct _player          PLAYER;
typedef struct _tutor           TUTOR;
typedef struct _zipprodq        ZIPPRODQ;

// Tile drawing callbacks use the native device-context handle type.
typedef void (*TILEDRAWPROC)(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);

// Win16 structs are byte-packed; keep the original field offsets.
// The codec harness also compiles these types with natural runtime alignment.
#ifndef STARS_TEST_NATURAL_RECORD_LAYOUT
#pragma pack(push, 1)
#endif

struct _aipart {
    uint16_t ibit : 4, /* +0x0000 (2) @bit0 */
        iItem : 5,     /* @bit4 */
        cItem : 4,     /* @bit9 */
        fRandom : 3;   /* @bit13 */
}; /* size=0x2 */

struct _aistarbase {
    int16_t idPlanet;   /* +0x0000 (2) */
    int16_t cFreighter; /* +0x0002 (2) */
    int16_t rgflid[8];  /* +0x0004 (16) */
}; /* size=0x14 */

struct _aihist {
    uint16_t   cbAiHist;  /* +0x0000 (2) */
    int16_t    cStarbase; /* +0x0002 (2) */
    AISTARBASE rgasb[64]; /* +0x0004 (1280) */
}; /* size=0x504 */

struct _armor {
    int16_t  id;             /* +0x0000 (2) */
    int8_t   rgTech[6];      /* +0x0002 (6) */
    char     szName[32];     /* +0x0008 (32) */
    int16_t  cMass;          /* +0x0028 (2) */
    uint16_t resCost;        /* +0x002A (2) */
    int16_t  rgwtOreCost[3]; /* +0x002C (6) */
    int16_t  ibmp;           /* +0x0032 (2) */
    int16_t  dp;             /* +0x0034 (2) */
}; /* size=0x36 */

struct _beam {
    int16_t     id;             /* +0x0000 (2) */
    int8_t      rgTech[6];      /* +0x0002 (6) */
    char        szName[32];     /* +0x0008 (32) */
    int16_t     cMass;          /* +0x0028 (2) */
    uint16_t    resCost;        /* +0x002A (2) */
    int16_t     rgwtOreCost[3]; /* +0x002C (6) */
    int16_t     ibmp;           /* +0x0032 (2) */
    int16_t     dRangeMax;      /* +0x0034 (2) */
    int16_t     dp;             /* +0x0036 (2) */
    int16_t     init;           /* +0x0038 (2) */
    BeamAbility grfAbilities;   /* +0x003A (2) */
}; /* size=0x3c */

struct _bomb {
    int16_t  id;             /* +0x0000 (2) */
    int8_t   rgTech[6];      /* +0x0002 (6) */
    char     szName[32];     /* +0x0008 (32) */
    int16_t  cMass;          /* +0x0028 (2) */
    uint16_t resCost;        /* +0x002A (2) */
    int16_t  rgwtOreCost[3]; /* +0x002C (6) */
    int16_t  ibmp;           /* +0x0032 (2) */
    int16_t  cRounds;        /* +0x0034 (2) */
    int16_t  dDmgCol;        /* +0x0036 (2) */
    int16_t  dDmgBldg;       /* +0x0038 (2) */
}; /* size=0x3a */

struct _btlplan {
    uint16_t iplr : 4,      /* +0x0000 (2) @bit0 */
        iplan : 4,          /* @bit4 */
        mdTactic : 4,       /* @bit8 */
        unused1 : 2,        /* @bit12 */
        fDelete : 1,        /* @bit14 */
        fDumpCargo : 1;     /* @bit15 */
    uint16_t mdTarget1 : 4, /* +0x0002 (2) @bit0 */
        mdTarget2 : 4,      /* @bit4 */
        iplrAttack : 5,     /* @bit8 */
        unused2 : 3;        /* @bit13 */
    char szName[32];        /* +0x0004 (32) */
}; /* size=0x24 */

struct _coldrop {
    int16_t  idFleetSrc;       /* +0x0000 (2) */
    int16_t  idPlr;            /* +0x0002 (2) */
    int16_t  idPlanetDst;      /* +0x0004 (2) */
    uint16_t fCanColonize : 1, /* +0x0006 (2) @bit0 */
        unused : 15;           /* @bit1 */
    int32_t cColonist;         /* +0x0008 (4) */
}; /* size=0xc */

struct _compart {
    int16_t  id;             /* +0x0000 (2) */
    int8_t   rgTech[6];      /* +0x0002 (6) */
    char     szName[32];     /* +0x0008 (32) */
    int16_t  cMass;          /* +0x0028 (2) */
    uint16_t resCost;        /* +0x002A (2) */
    int16_t  rgwtOreCost[3]; /* +0x002C (6) */
    int16_t  ibmp;           /* +0x0032 (2) */
}; /* size=0x34 */

struct _cyberinfo {
    uint16_t wInfo;
    constexpr uint16_t iLstPktDir() const noexcept { return (wInfo >> 0) & 0x7; }
    constexpr uint16_t fBltColony() const noexcept { return (wInfo >> 3) & 0x1; }
    constexpr uint16_t fLaunchedPkt() const noexcept { return (wInfo >> 4) & 0x1; }
    constexpr uint16_t iPktTarget() const noexcept { return (wInfo >> 5) & 0x3; }
    constexpr uint16_t fNeedScanPkt() const noexcept { return (wInfo >> 7) & 0x1; }
    constexpr uint16_t unused() const noexcept { return (wInfo >> 8) & 0xff; }
}; /* size=0x2 */

struct _cyberinfotemp {
    uint16_t wInfo1;
    constexpr uint16_t fIdleColonizers() const noexcept { return (wInfo1 >> 0) & 0x1; }
    constexpr uint16_t cIdleFreighters() const noexcept { return (wInfo1 >> 1) & 0x3; }
    constexpr uint16_t cFreightersDst() const noexcept { return (wInfo1 >> 3) & 0x3; }
    constexpr uint16_t fNeedDefenders() const noexcept { return (wInfo1 >> 5) & 0x1; }
    constexpr uint16_t fDefended() const noexcept { return (wInfo1 >> 6) & 0x1; }
    constexpr uint16_t fUnderAttack() const noexcept { return (wInfo1 >> 7) & 0x1; }
    constexpr uint16_t fNeedsMin1() const noexcept { return (wInfo1 >> 8) & 0x1; }
    constexpr uint16_t fNeedsMin2() const noexcept { return (wInfo1 >> 9) & 0x1; }
    constexpr uint16_t fNeedsMin3() const noexcept { return (wInfo1 >> 10) & 0x1; }
    constexpr uint16_t unused() const noexcept { return (wInfo1 >> 11) & 0x1f; }
}; /* size=0x2 */

struct _dv {
    uint16_t dp;
    constexpr uint16_t pctSh() const noexcept { return dp & 0x7f; }
    constexpr uint16_t pctDp() const noexcept { return dp >> 7; }
    constexpr uint16_t SetPctSh(uint16_t pctShNew) noexcept {
        dp = static_cast<uint16_t>((dp & 0xff80) | (pctShNew & 0x7f));
        return pctSh();
    }
    constexpr uint16_t SetPctDp(uint16_t pctDpNew) noexcept {
        dp = static_cast<uint16_t>((dp & 0x7f) | ((pctDpNew & 0x1ff) << 7));
        return pctDp();
    }
}; /* size=0x2 */

struct _engine {
    int16_t       id;              /* +0x0000 (2) */
    int8_t        rgTech[6];       /* +0x0002 (6) */
    char          szName[32];      /* +0x0008 (32) */
    int16_t       cMass;           /* +0x0028 (2) */
    uint16_t      resCost;         /* +0x002A (2) */
    int16_t       rgwtOreCost[3];  /* +0x002C (6) */
    int16_t       ibmp;            /* +0x0032 (2) */
    EngineAbility grfAbilities;    /* +0x0034 (2) */
    int16_t       rgcFuelUsed[12]; /* +0x0036 (24) */
}; /* size=0x4e */

struct _fleetid {
    uint16_t ifl : 9, /* +0x0000 (2) @bit0 */
        iplr : 4,     /* @bit9 */
        junk : 3;     /* @bit13 */
}; /* size=0x2 */

struct _framestuff {
    int16_t dx;          /* +0x0000 (2) */
    int16_t dy;          /* +0x0002 (2) */
    int16_t xTop;        /* +0x0004 (2) */
    int16_t y1;          /* +0x0006 (2) */
    int16_t y2;          /* +0x0008 (2) */
    int16_t dxPlanWant;  /* +0x000A (2) */
    int16_t dyMsgWant;   /* +0x000C (2) */
    int16_t dyMinWant;   /* +0x000E (2) */
    int16_t dx2PlanWant; /* +0x0010 (2) */
    int16_t dy2MsgWant;  /* +0x0012 (2) */
    int16_t dy2MinWant;  /* +0x0014 (2) */
}; /* size=0x16 */

struct _game {
    int32_t         lid;         /* +0x0000 (4) */
    UniverseSize    mdSize;      /* +0x0004 (2) */
    UniverseDensity mdDensity;   /* +0x0006 (2) */
    int16_t         cPlayer;     /* +0x0008 (2) */
    int16_t         cPlanMax;    /* +0x000A (2) */
    StartDistance   mdStartDist; /* +0x000C (2) */
    int16_t         fDirty;      /* +0x000E (2) */
    uint16_t fExtraFuel : 1, /* +0x0010 (2) @bit0 */
        fSlowTech : 1,       /* @bit1 */
        fSinglePlr : 1,      /* @bit2 */
        fTutorial : 1,       /* @bit3 */
        fAisBand : 1,        /* @bit4 */
        fBBSPlay : 1,        /* @bit5 */
        fVisScores : 1,      /* @bit6 */
        fNoRandom : 1,       /* @bit7 */
        fClumping : 1,       /* @bit8 */
        wGen : 3,            /* @bit9 */
        unused : 4;          /* @bit12 */
    uint16_t wCrap() const noexcept {
        return static_cast<uint16_t>(fExtraFuel | (fSlowTech << 1) | (fSinglePlr << 2) | (fTutorial << 3) | (fAisBand << 4) |
            (fBBSPlay << 5) | (fVisScores << 6) | (fNoRandom << 7) | (fClumping << 8) | (wGen << 9) | (unused << 12));
    }
    uint16_t turn;       /* +0x0012 (2) */
    uint8_t  rgvc[12];   /* +0x0014 (12) */
    char     szName[32]; /* +0x0020 (32) */
}; /* size=0x40 */

struct _gdata {
    uint16_t fUnknownShip : 1, /* +0x0000 (2) @bit0 */
        fGeneratingTurn : 1,   /* @bit1 */
        fForceTurn : 1,        /* @bit2 */
        fHostMode : 1,         /* @bit3 */
        fSubmit : 1,           /* @bit4 */
        fNoResearchSav : 1,    /* @bit5 */
        fRadiatingEngine : 1,  /* @bit6 */
        fNoIdleChecks : 1,     /* @bit7 */
        fSendMsgMode : 1,      /* @bit8 */
        fRetryOpens : 1,       /* @bit9 */
        fAisDone : 1,          /* @bit10 */
        fTutorial : 1,         /* @bit11 */
        fGotoVCR : 1,          /* @bit12 */
        fVCRTimer : 1,         /* @bit13 */
        mdScreenSize : 2;      /* @bit14 */
    uint16_t fGameOverMan : 1, /* +0x0002 (2) @bit0 */
        fDontDoLogFiles : 1,   /* @bit1 */
        fFileCrippled : 1,     /* @bit2 */
        fSmallTileMode : 1,    /* @bit3 */
        fAllAis : 1,           /* @bit4 */
        fReadOnly : 1,         /* @bit5 */
        fExitWindows : 1,      /* @bit6 */
        fPartialTurn : 1,      /* @bit7 */
        fSetMassMode : 1,      /* @bit8 */
        fRptSafeDraw : 1,      /* @bit9 */
        fProgressTxt : 1,      /* @bit10 */
        fSoundFX : 1,          /* @bit11 */
        fNoSound : 1,          /* @bit12 */
        fSetRouteMode : 1,     /* @bit13 */
        fBleedingEdge : 1,     /* @bit14 */
        fToolbar : 1;          /* @bit15 */
    int32_t grBits() const noexcept {
        const uint32_t w = uint32_t(fUnknownShip) | uint32_t(fGeneratingTurn) << 1 | uint32_t(fForceTurn) << 2 |
            uint32_t(fHostMode) << 3 | uint32_t(fSubmit) << 4 | uint32_t(fNoResearchSav) << 5 | uint32_t(fRadiatingEngine) << 6 |
            uint32_t(fNoIdleChecks) << 7 | uint32_t(fSendMsgMode) << 8 | uint32_t(fRetryOpens) << 9 | uint32_t(fAisDone) << 10 |
            uint32_t(fTutorial) << 11 | uint32_t(fGotoVCR) << 12 | uint32_t(fVCRTimer) << 13 | uint32_t(mdScreenSize) << 14 |
            uint32_t(fGameOverMan) << 16 | uint32_t(fDontDoLogFiles) << 17 | uint32_t(fFileCrippled) << 18 |
            uint32_t(fSmallTileMode) << 19 | uint32_t(fAllAis) << 20 | uint32_t(fReadOnly) << 21 | uint32_t(fExitWindows) << 22 |
            uint32_t(fPartialTurn) << 23 | uint32_t(fSetMassMode) << 24 | uint32_t(fRptSafeDraw) << 25 | uint32_t(fProgressTxt) << 26 |
            uint32_t(fSoundFX) << 27 | uint32_t(fNoSound) << 28 | uint32_t(fSetRouteMode) << 29 | uint32_t(fBleedingEdge) << 30 |
            uint32_t(fToolbar) << 31;
        return std::bit_cast<int32_t>(w);
    }
    void SetGrBits(int32_t grBits) noexcept {
        const uint32_t w = std::bit_cast<uint32_t>(grBits);
        fUnknownShip = w & 1u;
        fGeneratingTurn = (w >> 1) & 1u;
        fForceTurn = (w >> 2) & 1u;
        fHostMode = (w >> 3) & 1u;
        fSubmit = (w >> 4) & 1u;
        fNoResearchSav = (w >> 5) & 1u;
        fRadiatingEngine = (w >> 6) & 1u;
        fNoIdleChecks = (w >> 7) & 1u;
        fSendMsgMode = (w >> 8) & 1u;
        fRetryOpens = (w >> 9) & 1u;
        fAisDone = (w >> 10) & 1u;
        fTutorial = (w >> 11) & 1u;
        fGotoVCR = (w >> 12) & 1u;
        fVCRTimer = (w >> 13) & 1u;
        mdScreenSize = (w >> 14) & 3u;
        fGameOverMan = (w >> 16) & 1u;
        fDontDoLogFiles = (w >> 17) & 1u;
        fFileCrippled = (w >> 18) & 1u;
        fSmallTileMode = (w >> 19) & 1u;
        fAllAis = (w >> 20) & 1u;
        fReadOnly = (w >> 21) & 1u;
        fExitWindows = (w >> 22) & 1u;
        fPartialTurn = (w >> 23) & 1u;
        fSetMassMode = (w >> 24) & 1u;
        fRptSafeDraw = (w >> 25) & 1u;
        fProgressTxt = (w >> 26) & 1u;
        fSoundFX = (w >> 27) & 1u;
        fNoSound = (w >> 28) & 1u;
        fSetRouteMode = (w >> 29) & 1u;
        fBleedingEdge = (w >> 30) & 1u;
        fToolbar = (w >> 31) & 1u;
    }
    uint16_t fNoScannerDraw : 1, /* +0x0004 (2) @bit0 */
        fTrialPeriodOver : 1,    /* @bit1 */
        fClose : 1,              /* @bit2 */
        fDontCalcBleed : 1,      /* @bit3 */
        fChgZipOrd : 1,          /* @bit4 */
        fChgZipProd : 1,         /* @bit5 */
        fChgScanner : 1,         /* @bit6 */
        fChgReports : 1,         /* @bit7 */
        fWriteTurnNum : 1,       /* @bit8 */
        fHotSeat : 1,            /* @bit9 */
        fFleetLinkValid : 1,     /* @bit10 */
        fScoreVictory : 2,       /* @bit11 */
        unused3 : 3;             /* @bit13 */
    uint16_t iCurGraph : 4,      /* +0x0006 (2) @bit0 */
        fMusic : 1,              /* @bit4 */
        fPerPlayerDumps : 1,     /* @bit5 */
        fNoHostNames : 1,        /* @bit6 */
        unused4 : 9;             /* @bit7 */
    int32_t grBits2() const noexcept {
        const uint32_t w = uint32_t(fNoScannerDraw) | uint32_t(fTrialPeriodOver) << 1 | uint32_t(fClose) << 2 |
            uint32_t(fDontCalcBleed) << 3 | uint32_t(fChgZipOrd) << 4 | uint32_t(fChgZipProd) << 5 | uint32_t(fChgScanner) << 6 |
            uint32_t(fChgReports) << 7 | uint32_t(fWriteTurnNum) << 8 | uint32_t(fHotSeat) << 9 | uint32_t(fFleetLinkValid) << 10 |
            uint32_t(fScoreVictory) << 11 | uint32_t(unused3) << 13 | uint32_t(iCurGraph) << 16 | uint32_t(fMusic) << 20 |
            uint32_t(fPerPlayerDumps) << 21 | uint32_t(fNoHostNames) << 22 | uint32_t(unused4) << 23;
        return std::bit_cast<int32_t>(w);
    }
    void SetGrBits2(int32_t grBits2) noexcept {
        const uint32_t w = std::bit_cast<uint32_t>(grBits2);
        fNoScannerDraw = w & 1u;
        fTrialPeriodOver = (w >> 1) & 1u;
        fClose = (w >> 2) & 1u;
        fDontCalcBleed = (w >> 3) & 1u;
        fChgZipOrd = (w >> 4) & 1u;
        fChgZipProd = (w >> 5) & 1u;
        fChgScanner = (w >> 6) & 1u;
        fChgReports = (w >> 7) & 1u;
        fWriteTurnNum = (w >> 8) & 1u;
        fHotSeat = (w >> 9) & 1u;
        fFleetLinkValid = (w >> 10) & 1u;
        fScoreVictory = (w >> 11) & 3u;
        unused3 = (w >> 13) & 7u;
        iCurGraph = (w >> 16) & 15u;
        fMusic = (w >> 20) & 1u;
        fPerPlayerDumps = (w >> 21) & 1u;
        fNoHostNames = (w >> 22) & 1u;
        unused4 = (w >> 23) & 511u;
    }
    uint16_t fUnused2 : 14; /* +0x0008 (2) @bit0 */
}; /* size=0xa */

struct _hb {
    uint16_t cbFree;   /* +0x0000 (2) */
    uint16_t cbBlock;  /* +0x0002 (2) */
    uint16_t cbSlop;   /* +0x0004 (2) */
    uint16_t ibTop;    /* +0x0006 (2) */
    HB      *lphbNext; /* +0x0008 (4) */
    void    *hmem;     /* +0x000C (2) NATIVE: the C heap block */
    uint8_t  ht;       /* +0x000E (1) */
    uint8_t  unused1;  /* +0x000F (1) */
}; /* size=0x10 */

struct _hdr {
    uint16_t cb : 10, /* +0x0000 (2) @bit0 */
        rt : 6;       /* @bit10 */
}; /* size=0x2 */

struct _hs {
    HullSlotType grhst;     /* +0x0000 (2) */
    uint16_t     iItem : 8, /* +0x0002 (2) @bit0 */
        cItem : 8;          /* @bit8 */
}; /* size=0x4 */

struct _hul {
    HulDef   ihuldef;        /* +0x0000 (2) */
    int8_t   rgTech[6];      /* +0x0002 (6) */
    char     szClass[32];    /* +0x0008 (32) */
    uint16_t wtEmpty;        /* +0x0028 (2) */
    uint16_t resCost;        /* +0x002A (2) */
    uint16_t rgwtOreCost[3]; /* +0x002C (6) */
    int16_t  ibmp;           /* +0x0032 (2) */
    uint16_t wtCargoMax;     /* +0x0034 (2) */
    uint16_t wtFuelMax;      /* +0x0036 (2) */
    uint16_t dp;             /* +0x0038 (2) */
    HS       rghs[16];       /* +0x003A (64) */
    uint8_t  chs;            /* +0x007A (1) */
}; /* size=0x7b */

struct _huldef {
    HUL      hul;        /* +0x0000 (123) */
    uint16_t init : 6,   /* +0x007B (2) @bit0 */
        imdAttack : 4,   /* @bit6 */
        imdCategory : 4, /* @bit10 */
        unused : 2;      /* @bit14 */
    uint16_t wrcCargo;   /* +0x007D (2) */
    uint8_t  rgbrc[16];  /* +0x007F (16) */
}; /* size=0x8f */

struct alignas(2) _itemaction {
    uint16_t w;

    constexpr uint16_t cQuan() const noexcept { return w & 0x0fff; }
    constexpr uint16_t iAction() const noexcept { return w >> 12; }
    constexpr uint16_t SetCQuan(uint16_t cQuanNew) noexcept {
        w = static_cast<uint16_t>((w & 0xf000) | (cQuanNew & 0x0fff));
        return cQuan();
    }
    constexpr uint16_t SetIAction(uint16_t iActionNew) noexcept {
        w = static_cast<uint16_t>((w & 0x0fff) | ((iActionNew & 0x0f) << 12));
        return iAction();
    }
}; /* size=0x2 */

inline constexpr ITEMACTION IaMake(uint16_t cQuan, uint16_t iAction) noexcept {
    return {static_cast<uint16_t>((cQuan & 0x0fff) | ((iAction & 0x0f) << 12))};
}

struct _kill {
    uint8_t  itok;      /* +0x0000 (1) */
    uint8_t  grfWeapon; /* +0x0001 (1) */
    uint16_t cshKill;   /* +0x0002 (2) */
    uint16_t dpShield;  /* +0x0004 (2) */
    DV       dv;        /* +0x0006 (2) */
}; /* size=0x8 */

struct _btlrec {
    uint8_t  itok;       /* +0x0000 (1) */
    uint8_t  brcDest;    /* +0x0001 (1) */
    int16_t  ctok;       /* +0x0002 (2) */
    uint16_t iRound : 4, /* +0x0004 (2) @bit0 */
        dzDis : 4,       /* @bit4 */
        itokAttack : 8;  /* @bit8 */
}; /* size=0x6 */

struct _btlrec26 {
    uint8_t  itok;       /* +0x0000 (1) */
    uint8_t  brcDest;    /* +0x0001 (1) */
    uint8_t  itokAttack; /* +0x0002 (1) */
    uint8_t  ctok;       /* +0x0003 (1) */
    uint16_t iRound : 4, /* +0x0004 (2) @bit0 */
        dzDis : 4,       /* @bit4 */
        unused : 8;      /* @bit8 */
}; /* size=0x6 */

struct _logxfer {
    int16_t    id;         /* +0x0000 (2) */
    GrobjClass grobj;      /* +0x0002 (2) */
    int32_t    rgdItem[5]; /* +0x0004 (20) */
}; /* size=0x18 */

struct _logxferf {
    int16_t    id;          /* +0x0000 (2) */
    GrobjClass grobj;       /* +0x0002 (2) */
    int16_t    rgdItem[16]; /* +0x0004 (32) */
}; /* size=0x24 */

struct _lsb {
    uint16_t isb : 4,      /* +0x0000 (2) @bit0 */
        pctDp : 12;        /* @bit4 */
    uint16_t idFling : 10, /* +0x0002 (2) @bit0 */
        iWarpFling : 4,    /* @bit10 */
        unused3 : 2;       /* @bit14 */
}; /* size=0x4 */

struct _mdplr {
    uint16_t reserved : 9, /* +0x0000 (2) @bit0 */
        fAi : 1,           /* @bit9 */
        lvlAi : 3,         /* @bit10 */
        idAi : 3;          /* @bit13 */
    uint16_t wMdPlr() const noexcept {
        return static_cast<uint16_t>(reserved | (fAi << 9) | (lvlAi << 10) | (idAi << 13));
    }
    void SetWMdPlr(uint16_t wMdPlr) noexcept {
        reserved = wMdPlr & 511; fAi = (wMdPlr >> 9) & 1; lvlAi = (wMdPlr >> 10) & 7; idAi = wMdPlr >> 13;
    }
}; /* size=0x2 */

struct _mines {
    int16_t  id;             /* +0x0000 (2) */
    int8_t   rgTech[6];      /* +0x0002 (6) */
    char     szName[32];     /* +0x0008 (32) */
    int16_t  cMass;          /* +0x0028 (2) */
    uint16_t resCost;        /* +0x002A (2) */
    int16_t  rgwtOreCost[3]; /* +0x002C (6) */
    int16_t  ibmp;           /* +0x0032 (2) */
    int16_t  grAbility;      /* +0x0034 (2) */
}; /* size=0x36 */

struct _mining {
    int16_t  id;             /* +0x0000 (2) */
    int8_t   rgTech[6];      /* +0x0002 (6) */
    char     szName[32];     /* +0x0008 (32) */
    int16_t  cMass;          /* +0x0028 (2) */
    uint16_t resCost;        /* +0x002A (2) */
    int16_t  rgwtOreCost[3]; /* +0x002C (6) */
    int16_t  ibmp;           /* +0x0032 (2) */
    int16_t  grAbility;      /* +0x0034 (2) */
}; /* size=0x36 */

struct _msgbig {
    int16_t iMsg;       /* +0x0000 (2) */
    MsgGoto wGoto;      /* +0x0002 (2) */
    int16_t rgParam[7]; /* +0x0004 (14) */
}; /* size=0x12 */

struct _msghdr {
    uint16_t iMsg : 9, /* +0x0000 (2) @bit0 */
        grWord : 7;    /* @bit9 */
    MsgGoto wGoto;     /* +0x0002 (2) */
}; /* size=0x4 */

struct _msgplr {
    MSGPLR *lpmsgplrNext; /* +0x0000 (4) */
    int16_t iPlrFrom;     /* +0x0004 (2) */
    int16_t iPlrTo;       /* +0x0006 (2) */
    int16_t iInRe;        /* +0x0008 (2) */
    int16_t cLen;         /* +0x000A (2) */
    // The typed allocator starts an adjacent byte array with its own lifetime.
    uint8_t *rgbMsg() noexcept { return reinterpret_cast<uint8_t *>(this) + sizeof(*this); }
    const uint8_t *rgbMsg() const noexcept { return reinterpret_cast<const uint8_t *>(this) + sizeof(*this); }
}; /* size=0xc */

struct _msgturn {
    uint8_t iPlr : 4, /* +0x0000 (1) @bit0 */
        cbParams : 4; /* @bit4 */
    MSGHDR msghdr;    /* +0x0001 (4) */
}; /* size=0x5 */

struct _obj {
    // Drawing callbacks borrow the object chosen by their tile group.
    // Converting through void preserves that pointer without an inactive union.
    void *lpv;
    PLANET *ppl() const noexcept { return static_cast<PLANET *>(lpv); }
    FLEET *pfl() const noexcept { return static_cast<FLEET *>(lpv); }
    THING *pth() const noexcept { return static_cast<THING *>(lpv); }
    void SetPpl(PLANET *ppl) noexcept { lpv = ppl; }
    void SetPfl(FLEET *pfl) noexcept { lpv = pfl; }
    void SetPth(THING *pth) noexcept { lpv = pth; }
}; /* Runtime borrow; never serialized. */

enum PartPointerKind : uint8_t { ppkCommon, ppkArmor, ppkHull, ppkEngine, ppkScanner, ppkBeam, ppkTorp, ppkBomb, ppkShield, ppkSpecial, ppkSpecialSB, ppkMines, ppkMining, ppkPlanetary, ppkTerra };
class NATIVEPARTCOMMONVIEW;

struct _part {
    HS hs;
private:
    void *lpv;
    PartPointerKind ppk;
public:
    // Retain the pointer kind independently of hs: a failed lookup leaves the
    // previous catalog borrow installed even after the caller changes hs.
    NATIVEPARTCOMMONVIEW pcom() const noexcept;
    void SetPcom(NATIVEPARTCOMMONVIEW pcom) noexcept;
    void SetPcom(COMPART *pcom) noexcept { lpv = pcom; ppk = ppkCommon; }
    ARMOR *parmor() const noexcept { return static_cast<ARMOR *>(lpv); }
    void SetParmor(ARMOR *parmor) noexcept { lpv = parmor; ppk = ppkArmor; }
    HUL *phul() const noexcept { return static_cast<HUL *>(lpv); }
    void SetPhul(HUL *phul) noexcept { lpv = phul; ppk = ppkHull; }
    ENGINE *pengine() const noexcept { return static_cast<ENGINE *>(lpv); }
    void SetPengine(ENGINE *pengine) noexcept { lpv = pengine; ppk = ppkEngine; }
    SCANNER *pscanner() const noexcept { return static_cast<SCANNER *>(lpv); }
    void SetPscanner(SCANNER *pscanner) noexcept { lpv = pscanner; ppk = ppkScanner; }
    BEAM *pbeam() const noexcept { return static_cast<BEAM *>(lpv); }
    void SetPbeam(BEAM *pbeam) noexcept { lpv = pbeam; ppk = ppkBeam; }
    TORP *ptorp() const noexcept { return static_cast<TORP *>(lpv); }
    void SetPtorp(TORP *ptorp) noexcept { lpv = ptorp; ppk = ppkTorp; }
    BOMB *pbomb() const noexcept { return static_cast<BOMB *>(lpv); }
    void SetPbomb(BOMB *pbomb) noexcept { lpv = pbomb; ppk = ppkBomb; }
    SHIELD *pshield() const noexcept { return static_cast<SHIELD *>(lpv); }
    void SetPshield(SHIELD *pshield) noexcept { lpv = pshield; ppk = ppkShield; }
    SPECIAL *pspecial() const noexcept { return static_cast<SPECIAL *>(lpv); }
    void SetPspecial(SPECIAL *pspecial) noexcept { lpv = pspecial; ppk = ppkSpecial; }
    SPECIALSB *pspecialsb() const noexcept { return static_cast<SPECIALSB *>(lpv); }
    void SetPspecialsb(SPECIALSB *pspecialsb) noexcept { lpv = pspecialsb; ppk = ppkSpecialSB; }
    MINES *pmines() const noexcept { return static_cast<MINES *>(lpv); }
    void SetPmines(MINES *pmines) noexcept { lpv = pmines; ppk = ppkMines; }
    MINING *pmining() const noexcept { return static_cast<MINING *>(lpv); }
    void SetPmining(MINING *pmining) noexcept { lpv = pmining; ppk = ppkMining; }
    PLANETARY *pplanetary() const noexcept { return static_cast<PLANETARY *>(lpv); }
    void SetPplanetary(PLANETARY *pplanetary) noexcept { lpv = pplanetary; ppk = ppkPlanetary; }
    TERRA *pterra() const noexcept { return static_cast<TERRA *>(lpv); }
    void SetPterra(TERRA *pterra) noexcept { lpv = pterra; ppk = ppkTerra; }
}; /* Runtime catalog borrow; never serialized. */

struct _pl {
    uint16_t cbItem : 8, /* +0x0000 (2) @bit0 */
        fMark : 1,       /* @bit8 */
        ht : 3,          /* @bit9 */
        cAlloc : 4;      /* @bit12 */
    uint8_t iMax;        /* +0x0002 (1) */
    uint8_t iMac;        /* +0x0003 (1) */
}; /* size=0x4 */

struct _planet {
    int16_t  id;              /* +0x0000 (2) */
    int16_t  iPlayer;         /* +0x0002 (2) */
    uint16_t det : 8,         /* +0x0004 (2) @bit0 */
        fInclude : 1,         /* @bit8 */
        fStarbase : 1,        /* @bit9 */
        fHomeworld : 1,       /* @bit10 */
        fFirstYear : 1,       /* @bit11 */
        unusedC : 1,          /* @bit12 */
        fWasInhabited : 1,    /* @bit13 */
        unusedD : 2;          /* @bit14 */
    uint8_t rgpctMinLevel[3]; /* +0x0006 (3) */
    uint8_t rgMinConc[3];     /* +0x0009 (3) */
    int8_t  rgEnvVar[3];      /* +0x000C (3) */
    int8_t  rgEnvVarOrig[3];  /* +0x000F (3) */
    uint16_t uPopGuess : 12, /* +0x0012 (2) @bit0 */
        uDefGuess : 4;       /* @bit12 */
    uint16_t uGuesses() const noexcept { return static_cast<uint16_t>(uPopGuess | (uDefGuess << 12)); }
    void SetUGuesses(uint16_t uGuesses) noexcept { uPopGuess = uGuesses & 4095; uDefGuess = uGuesses >> 12; }
    uint32_t iDeltaPop : 8,  /* +0x0014 (4) @bit0 */
        cMines : 12,         /* @bit8 */
        cFactories : 12;     /* @bit20 */
    uint32_t cDefenses : 12, /* +0x0018 (4) @bit0 */
        iScanner : 5,        /* @bit12 */
        unused5 : 5,         /* @bit17 */
        fArtifact : 1,       /* @bit22 */
        fNoResearch : 1,     /* @bit23 */
        unused2 : 8;         /* @bit24 */
    std::array<uint8_t, 8> rgbImp() const noexcept {
        const uint32_t ul1 = uint32_t(iDeltaPop) | (uint32_t(cMines) << 8) | (uint32_t(cFactories) << 20);
        const uint32_t ul2 = uint32_t(cDefenses) | (uint32_t(iScanner) << 12) | (uint32_t(unused5) << 17) |
            (uint32_t(fArtifact) << 22) | (uint32_t(fNoResearch) << 23) | (uint32_t(unused2) << 24);
        return {uint8_t(ul1), uint8_t(ul1 >> 8), uint8_t(ul1 >> 16), uint8_t(ul1 >> 24),
                uint8_t(ul2), uint8_t(ul2 >> 8), uint8_t(ul2 >> 16), uint8_t(ul2 >> 24)};
    }
    void SetRgbImp(const std::array<uint8_t, 8> &rgbImp) noexcept {
        const uint32_t ul1 = uint32_t(rgbImp[0]) | (uint32_t(rgbImp[1]) << 8) | (uint32_t(rgbImp[2]) << 16) | (uint32_t(rgbImp[3]) << 24);
        const uint32_t ul2 = uint32_t(rgbImp[4]) | (uint32_t(rgbImp[5]) << 8) | (uint32_t(rgbImp[6]) << 16) | (uint32_t(rgbImp[7]) << 24);
        iDeltaPop = ul1 & 255; cMines = (ul1 >> 8) & 4095; cFactories = ul1 >> 20;
        cDefenses = ul2 & 4095; iScanner = (ul2 >> 12) & 31; unused5 = (ul2 >> 17) & 31;
        fArtifact = (ul2 >> 22) & 1; fNoResearch = (ul2 >> 23) & 1; unused2 = ul2 >> 24;
    }
    int32_t rgwtMin[4]; /* +0x001C (16) */
    uint16_t isb : 4,      /* +0x002C (2) @bit0 */
        pctDp : 12;        /* @bit4 */
    uint16_t idFling : 10, /* +0x002E (2) @bit0 */
        iWarpFling : 4,    /* @bit10 */
        fNoHeal : 1,       /* @bit14 */
        unused3 : 1;       /* @bit15 */
    int32_t lStarbase() const noexcept {
        const uint32_t ul = uint32_t(isb) | (uint32_t(pctDp) << 4) | (uint32_t(idFling) << 16) |
            (uint32_t(iWarpFling) << 26) | (uint32_t(fNoHeal) << 30) | (uint32_t(unused3) << 31);
        return std::bit_cast<int32_t>(ul);
    }
    void SetLStarbase(int32_t lStarbase) noexcept {
        const uint32_t ul = std::bit_cast<uint32_t>(lStarbase);
        isb = ul & 15; pctDp = (ul >> 4) & 4095; idFling = (ul >> 16) & 1023;
        iWarpFling = (ul >> 26) & 15; fNoHeal = (ul >> 30) & 1; unused3 = ul >> 31;
    }
    uint16_t idRoute : 10, /* +0x0030 (2) @bit0 */
        unused4 : 6;       /* @bit10 */
    uint16_t wRouting() const noexcept { return static_cast<uint16_t>(idRoute | (unused4 << 10)); }
    void SetWRouting(uint16_t wRouting) noexcept { idRoute = wRouting & 1023; unused4 = wRouting >> 10; }
    int16_t turn;     /* +0x0032 (2) */
    PLPROD *lpplprod; /* +0x0034 (4) */
}; /* size=0x38 */

struct _planetary {
    int16_t  id;             /* +0x0000 (2) */
    int8_t   rgTech[6];      /* +0x0002 (6) */
    char     szName[32];     /* +0x0008 (32) */
    int16_t  cMass;          /* +0x0028 (2) */
    uint16_t resCost;        /* +0x002A (2) */
    int16_t  rgwtOreCost[3]; /* +0x002C (6) */
    int16_t  ibmp;           /* +0x0032 (2) */
    int16_t  grAbility;      /* +0x0034 (2) */
}; /* size=0x36 */

struct _planetminimal {
    int16_t  id;        /* +0x0000 (2) */
    int16_t  iPlayer;   /* +0x0002 (2) */
    uint16_t det : 8,   /* +0x0004 (2) @bit0 */
        fInclude : 1,   /* @bit8 */
        fStarbase : 1,  /* @bit9 */
        unusedA : 1,    /* @bit10 */
        fFirstYear : 1, /* @bit11 */
        unusedB : 4;    /* @bit12 */
}; /* size=0x6 */

struct _planetsome {
    int16_t  id;               /* +0x0000 (2) */
    int16_t  iPlayer;          /* +0x0002 (2) */
    uint16_t det : 8,          /* +0x0004 (2) @bit0 */
        fInclude : 1,          /* @bit8 */
        fStarbase : 1,         /* @bit9 */
        unusedA : 1,           /* @bit10 */
        fFirstYear : 1,        /* @bit11 */
        unusedB : 4;           /* @bit12 */
    uint16_t rgpctMinLevel[3]; /* +0x0006 (6) */
    int8_t   rgMinConc[3];     /* +0x000C (3) */
    int8_t   rgEnvVar[3];      /* +0x000F (3) */
    int8_t   rgEnvVarOrig[3];  /* +0x0012 (3) */
    uint16_t uPopGuess : 12,
        uDefGuess : 4;
    uint16_t uGuesses() const noexcept { return static_cast<uint16_t>(uPopGuess | (uDefGuess << 12)); }
    void SetUGuesses(uint16_t uGuesses) noexcept { uPopGuess = uGuesses & 4095; uDefGuess = uGuesses >> 12; }
}; /* size=0x17 */

struct _fleet {
    int16_t id;
    constexpr uint16_t ifl() const noexcept { return static_cast<uint16_t>((static_cast<uint32_t>(id) >> 0) & 0x1ff); }
    constexpr uint16_t SetIfl(uint16_t iflNew) noexcept {
        id = static_cast<int16_t>((static_cast<uint32_t>(id) & 0xfe00u) | ((static_cast<uint32_t>(static_cast<uint16_t>(iflNew)) & 0x1ffu) << 0));
        return ifl();
    }
    constexpr uint16_t iplr() const noexcept { return static_cast<uint16_t>((static_cast<uint32_t>(id) >> 9) & 0xf); }
    constexpr uint16_t SetIplr(uint16_t iplrNew) noexcept {
        id = static_cast<int16_t>((static_cast<uint32_t>(id) & 0xe1ffu) | ((static_cast<uint32_t>(static_cast<uint16_t>(iplrNew)) & 0xfu) << 9));
        return iplr();
    }
    constexpr uint16_t junk() const noexcept { return static_cast<uint16_t>((static_cast<uint32_t>(id) >> 13) & 0x7); }
    constexpr uint16_t SetJunk(uint16_t junkNew) noexcept {
        id = static_cast<int16_t>((static_cast<uint32_t>(id) & 0x1fffu) | ((static_cast<uint32_t>(static_cast<uint16_t>(junkNew)) & 0x7u) << 13));
        return junk();
    }
    int16_t  iPlayer;     /* +0x0002 (2) */
    uint16_t det : 8,     /* +0x0004 (2) @bit0 */
        fInclude : 1,     /* @bit8 */
        fRepOrders : 1,   /* @bit9 */
        fDead : 1,        /* @bit10 */
        fDone : 1,        /* @bit11 */
        fBombed : 1,      /* @bit12 */
        fHereAllTurn : 1, /* @bit13 */
        fNoHeal : 1,      /* @bit14 */
        fMark : 1;        /* @bit15 */
    int16_t idPlanet;     /* +0x0006 (2) */
    POINT16 pt;           /* +0x0008 (4) */
    int16_t rgcsh[16];    /* +0x000C (32) */
    DV rgdv[16];
    int32_t wtFleet() const noexcept {
        return static_cast<int32_t>(static_cast<uint32_t>(rgdv[0].dp) | (static_cast<uint32_t>(rgdv[1].dp) << 16));
    }
    void SetWtFleet(int32_t wtFleetNew) noexcept {
        rgdv[0].dp = static_cast<uint16_t>(wtFleetNew);
        rgdv[1].dp = static_cast<uint16_t>(static_cast<uint32_t>(wtFleetNew) >> 16);
    }
    int32_t rgwtMin[5]; /* +0x004C (20) */
    uint8_t iplan;      /* +0x0060 (1) */
    uint8_t bUnused;    /* +0x0061 (1) */
    int16_t cord;       /* +0x0062 (2) */
    PLORD  *lpplord;    /* +0x0064 (4) */
    FLEET  *lpflNext;   /* +0x0068 (4) */
    int32_t lPower;
    constexpr int16_t dMoveLeft() const noexcept { return static_cast<int16_t>((static_cast<uint32_t>(lPower) >> 0) & 0xffff); }
    constexpr int16_t SetDMoveLeft(int16_t dMoveLeftNew) noexcept {
        lPower = static_cast<int32_t>((static_cast<uint32_t>(lPower) & 0xffff0000u) | ((static_cast<uint32_t>(static_cast<uint16_t>(dMoveLeftNew)) & 0xffffu) << 0));
        return dMoveLeft();
    }
    constexpr int16_t dMoveUsed() const noexcept { return static_cast<int16_t>((static_cast<uint32_t>(lPower) >> 16) & 0xffff); }
    constexpr int16_t SetDMoveUsed(int16_t dMoveUsedNew) noexcept {
        lPower = static_cast<int32_t>((static_cast<uint32_t>(lPower) & 0xffffu) | ((static_cast<uint32_t>(static_cast<uint16_t>(dMoveUsedNew)) & 0xffffu) << 16));
        return dMoveUsed();
    }
    int32_t lFuelUsed; /* +0x0070 (4) */
    int32_t dirLong;
    constexpr uint16_t dirFltX() const noexcept { return static_cast<uint16_t>((static_cast<uint32_t>(dirLong) >> 0) & 0xff); }
    constexpr uint16_t SetDirFltX(uint16_t dirFltXNew) noexcept {
        dirLong = static_cast<int32_t>((static_cast<uint32_t>(dirLong) & 0xffffff00u) | ((static_cast<uint32_t>(static_cast<uint16_t>(dirFltXNew)) & 0xffu) << 0));
        return dirFltX();
    }
    constexpr uint16_t dirFltY() const noexcept { return static_cast<uint16_t>((static_cast<uint32_t>(dirLong) >> 8) & 0xff); }
    constexpr uint16_t SetDirFltY(uint16_t dirFltYNew) noexcept {
        dirLong = static_cast<int32_t>((static_cast<uint32_t>(dirLong) & 0xffff00ffu) | ((static_cast<uint32_t>(static_cast<uint16_t>(dirFltYNew)) & 0xffu) << 8));
        return dirFltY();
    }
    constexpr uint16_t iwarpFlt() const noexcept { return static_cast<uint16_t>((static_cast<uint32_t>(dirLong) >> 16) & 0xf); }
    constexpr uint16_t SetIwarpFlt(uint16_t iwarpFltNew) noexcept {
        dirLong = static_cast<int32_t>((static_cast<uint32_t>(dirLong) & 0xfff0ffffu) | ((static_cast<uint32_t>(static_cast<uint16_t>(iwarpFltNew)) & 0xfu) << 16));
        return iwarpFlt();
    }
    constexpr uint16_t fdirValid() const noexcept { return static_cast<uint16_t>((static_cast<uint32_t>(dirLong) >> 20) & 0x1); }
    constexpr uint16_t SetFdirValid(uint16_t fdirValidNew) noexcept {
        dirLong = static_cast<int32_t>((static_cast<uint32_t>(dirLong) & 0xffefffffu) | ((static_cast<uint32_t>(static_cast<uint16_t>(fdirValidNew)) & 0x1u) << 20));
        return fdirValid();
    }
    constexpr uint16_t fCompChg() const noexcept { return static_cast<uint16_t>((static_cast<uint32_t>(dirLong) >> 21) & 0x1); }
    constexpr uint16_t SetFCompChg(uint16_t fCompChgNew) noexcept {
        dirLong = static_cast<int32_t>((static_cast<uint32_t>(dirLong) & 0xffdfffffu) | ((static_cast<uint32_t>(static_cast<uint16_t>(fCompChgNew)) & 0x1u) << 21));
        return fCompChg();
    }
    constexpr uint16_t fTargeted() const noexcept { return static_cast<uint16_t>((static_cast<uint32_t>(dirLong) >> 22) & 0x1); }
    constexpr uint16_t SetFTargeted(uint16_t fTargetedNew) noexcept {
        dirLong = static_cast<int32_t>((static_cast<uint32_t>(dirLong) & 0xffbfffffu) | ((static_cast<uint32_t>(static_cast<uint16_t>(fTargetedNew)) & 0x1u) << 22));
        return fTargeted();
    }
    constexpr uint16_t fSkipped() const noexcept { return static_cast<uint16_t>((static_cast<uint32_t>(dirLong) >> 23) & 0x1); }
    constexpr uint16_t SetFSkipped(uint16_t fSkippedNew) noexcept {
        dirLong = static_cast<int32_t>((static_cast<uint32_t>(dirLong) & 0xff7fffffu) | ((static_cast<uint32_t>(static_cast<uint16_t>(fSkippedNew)) & 0x1u) << 23));
        return fSkipped();
    }
    constexpr uint16_t fUnused() const noexcept { return static_cast<uint16_t>((static_cast<uint32_t>(dirLong) >> 24) & 0xff); }
    constexpr uint16_t SetFUnused(uint16_t fUnusedNew) noexcept {
        dirLong = static_cast<int32_t>((static_cast<uint32_t>(dirLong) & 0xffffffu) | ((static_cast<uint32_t>(static_cast<uint16_t>(fUnusedNew)) & 0xffu) << 24));
        return fUnused();
    }
    char *lpszName; /* +0x0078 (4) */
}; /* size=0x7c */

struct _fleetsome {
    int16_t  id;        /* +0x0000 (2) */
    int16_t  iPlayer;   /* +0x0002 (2) */
    uint16_t det : 8,   /* +0x0004 (2) @bit0 */
        fInclude : 1,   /* @bit8 */
        fRepOrders : 1, /* @bit9 */
        fDead : 1,      /* @bit10 */
        fByteCsh : 1,   /* @bit11 */
        unused : 4;     /* @bit12 */
    int16_t idPlanet;   /* +0x0006 (2) */
    POINT16 pt;         /* +0x0008 (4) */
}; /* size=0xc */

struct _popupdata {
    GrPopupType grPopup;
    int32_t rgi[5];
    int16_t idPlanet, iPlanetVar, iPlanVal, iPlanMin, iPlanMax, iPlrVal, iPlrMin, iPlrMax;
    int16_t idPlan, cMax, cCur, cOperate, fFactory;
    int16_t dxOut;
    char *psz;
    int16_t iPlayer;
    FLEET *lpfl;
    int16_t fRedDamage, dxDamage;
    uint16_t grbit;
    PART part;
    SHDEF *lpshdef;
    int16_t fShowDamage, fHideCounts, fToken, fSummary, itok;

    // The factory/mine display reads the old shared grbit word. Preserve its
    // numeric carry between popup kinds without overlaying borrowed pointers.
    // Other payloads are actual fields; their original scalar widths stay.
    void SetRgi(int i, int32_t value) noexcept {
        rgi[i] = value;
        if (i == static_cast<int>((sizeof(void *) + 4) / 4)) grbit = static_cast<uint16_t>(value);
    }
    void SetIPlrMin(int16_t value) noexcept {
        iPlrMin = value;
        if constexpr (sizeof(void *) == 8) grbit = static_cast<uint16_t>(value);
    }
    void SetIPlanMax(int16_t value) noexcept {
        iPlanMax = value;
        if constexpr (sizeof(void *) == 4) grbit = static_cast<uint16_t>(value);
    }
    void SetFFactory(int16_t value) noexcept {
        fFactory = value;
        if constexpr (sizeof(void *) == 4) grbit = static_cast<uint16_t>(value);
    }
    void SetFToken(int16_t value) noexcept { fToken = value; grbit = static_cast<uint16_t>(value); }
}; /* Runtime popup state, never serialized. */

struct _prod {
    uint32_t cItem : 10, /* +0x0000 (4) @bit0 */
        iItem : 7,       /* @bit10 */
        grobj : 3,       /* @bit17 */
        pct : 7,         /* @bit20 */
        unused : 5;      /* @bit27 */
}; /* size=0x4 */

struct PLPROD {
    uint16_t cbItem : 8, /* +0x0000 (2) @bit0 */
        fMark : 1,       /* @bit8 */
        ht : 3,          /* @bit9 */
        cAlloc : 4;      /* @bit12 */
    uint8_t iprodMax;    /* +0x0002 (1) */
    uint8_t iprodMac;    /* +0x0003 (1) */
    // The adjacent PROD array has a separate lifetime, started by the typed
    // pooled allocator. Keep the four-byte header and original slot stride.
    PROD *rgprod() noexcept { return reinterpret_cast<PROD *>(reinterpret_cast<uint8_t *>(this) + 4); }
    const PROD *rgprod() const noexcept { return reinterpret_cast<const PROD *>(reinterpret_cast<const uint8_t *>(this) + 4); }
}; /* size=0x4 */

struct _prodq1 {
    uint16_t w; /* +0x0000 (2) */
    uint16_t mdIdle() const noexcept { return w & 63; }
    uint16_t cQuan() const noexcept { return w >> 6; }
    void SetMdIdle(uint16_t mdIdle) noexcept { w = (w & 0xffc0u) | (mdIdle & 63); }
    void SetCQuan(uint16_t cQuan) noexcept { w = (w & 63u) | ((cQuan & 1023) << 6); }
}; /* size=0x2 */

inline constexpr PRODQ1 PqMake(uint16_t mdIdle, uint16_t cQuan) noexcept {
    return {static_cast<uint16_t>((mdIdle & 63) | ((cQuan & 1023) << 6))};
}

struct _btn {
    RECT     rc;           /* +0x0000 (8) */
    int16_t  bt;           /* +0x0008 (2) */
    int16_t  iVal;         /* +0x000A (2) */
    uint16_t fVisible : 1, /* +0x000C (2) @bit0 */
        fDisabled : 1,     /* @bit1 */
        iSide : 2,         /* @bit2 */
        fUnused : 12;      /* @bit4 */
}; /* size=0xe */

struct _btnt {
    HWND     hwnd;        /* +0x0000 (2) */
    HDC      hdc;         /* +0x0002 (2) */
    RECT     rc;          /* +0x0004 (8) */
    int16_t  dTimer;      /* +0x000C (2) */
    int16_t  btf;         /* +0x000E (2) */
    const char *szText;   /* +0x0010 (2) */
    int32_t  lTicks;      /* +0x0012 (4) */
    uint16_t fFirst : 1,  /* +0x0016 (2) @bit0 */
        fDown : 1,        /* @bit1 */
        fInitDown : 1,    /* @bit2 */
        fCreatedDC : 1,   /* @bit3 */
        fNoEndRedraw : 1, /* @bit4 */
        fUnused : 11;     /* @bit5 */
}; /* size=0x18 */

struct _drawcir {
    int16_t *rgx;        /* +0x0000 (2) */
    int16_t *rgy;        /* +0x0002 (2) */
    int16_t *rgrad;      /* +0x0004 (2) */
    int16_t  cCur;       /* +0x0006 (2) */
    int16_t  cMax;       /* +0x0008 (2) */
    HDC      hdc;        /* +0x000A (2) */
    RECT     rcClip;     /* +0x000C (8) */
    int16_t  fCovered;   /* +0x0014 (2) */
    int16_t  fHollowOut; /* +0x0016 (2) */
}; /* size=0x18 */

struct _rpt {
    int32_t    grbitVisible; /* +0x0000 (4) */
    ReportType irpt;         /* +0x0004 (2) */
    int16_t    cFields;      /* +0x0006 (2) */
    int16_t    cFieldFirst;  /* +0x0008 (2) */
    int16_t    icolSort;     /* +0x000A (2) */
    int16_t    fAscending;   /* +0x000C (2) */
    int16_t    irowFirst;    /* +0x000E (2) */
    POINT16    ptDlg;        /* +0x0010 (4) */
    POINT16    ptSize;       /* +0x0014 (4) */
    int16_t    fCached;      /* +0x0018 (2) */
    uint8_t    rgbdx[16];    /* +0x001A (16) */
    int16_t    cRows;        /* +0x002A (2) */
    int16_t    cRowsVis;     /* +0x002C (2) */
    int16_t    iSubsort;     /* +0x002E (2) */
    HWND       hwndVScroll;  /* +0x0030 (2) */
    HWND       hwndHScroll;  /* +0x0032 (2) */
    int16_t    cColScroll;   /* +0x0034 (2) */
}; /* size=0x36 */

struct _rtbof {
    char    rgid[4]; /* +0x0000 (4) */
    int32_t lidGame; /* +0x0004 (4) */
    uint16_t verInc : 5, /* +0x0008 (2) @bit0 */
        verMinor : 7,    /* @bit5 */
        verMajor : 4;    /* @bit12 */
    uint16_t wVersion() const noexcept { return static_cast<uint16_t>(verInc | (verMinor << 5) | (verMajor << 12)); }
    uint16_t turn;        /* +0x000A (2) */
    int16_t  iPlayer : 5, /* +0x000C (2) @bit0 */
        lSaltTime : 11;   /* @bit5 */
    uint16_t dt : 8,      /* +0x000E (2) @bit0 */
        fDone : 1,        /* @bit8 */
        fInUse : 1,       /* @bit9 */
        fMulti : 1,       /* @bit10 */
        fGameOverMan : 1, /* @bit11 */
        fCrippled : 1,    /* @bit12 */
        wGen : 3;         /* @bit13 */
}; /* size=0x10 */

struct _rtchgname {
    int16_t id;      /* +0x0000 (2) */
    int16_t grobj;   /* +0x0002 (2) */
    uint8_t rgb[33]; /* +0x0004 (33) */
}; /* size=0x25 */

struct _rtChgPlanetLong {
    int16_t id; /* +0x0000 (2) */
    uint32_t fNoResearch : 1, /* +0x0002 (4) @bit0 */
        idFling : 10,         /* @bit1 */
        iWarpFling : 4,       /* @bit11 */
        idRoute : 10,         /* @bit15 */
        unused : 7;           /* @bit25 */
    uint32_t ul() const noexcept {
        return uint32_t(fNoResearch) | (uint32_t(idFling) << 1) | (uint32_t(iWarpFling) << 11) |
            (uint32_t(idRoute) << 15) | (uint32_t(unused) << 25);
    }
}; /* size=0x6 */

struct _rtChgProdQ {
    int16_t id;        /* +0x0000 (2) */
    // Queue items are decoded from the following bytes; they are not members.
}; /* size=0x2 */

struct _rthisthdr {
    int16_t cPlanet;      /* +0x0000 (2) */
    int16_t cPlanetExtra; /* +0x0002 (2) */
}; /* size=0x4 */

struct _rtloghdr {
    int16_t cbLog;         /* +0x0000 (2) */
    uint8_t rgbUnused[15]; /* +0x0002 (15) */
}; /* size=0x11 */

struct _rtlogthing {
    uint16_t idFull;    /* +0x0000 (2) */
    int16_t  fDetonate; /* +0x0002 (2) */
}; /* size=0x4 */

struct _rtplanet {
    int16_t id : 11,     /* +0x0000 (2) @bit0 */
        iPlayer : 5;     /* @bit11 */
    uint16_t det : 7,    /* +0x0002 (2) @bit0 */
        fHomeworld : 1,  /* @bit7 */
        fInclude : 1,    /* @bit8 */
        fStarbase : 1,   /* @bit9 */
        fIncEVO : 1,     /* @bit10 */
        fIncImp : 1,     /* @bit11 */
        fIsArtifact : 1, /* @bit12 */
        fIncSurfMin : 1, /* @bit13 */
        fRouting : 1,    /* @bit14 */
        fFirstYear : 1;  /* @bit15 */
}; /* size=0x4 */

struct _rtshdef {
    uint16_t det : 8, /* Detail byte; encoded independently of native bitfield layout. */
        fInclude : 1,
        fFree : 1,
        ishdef : 5,
        fGift : 1;
    constexpr uint16_t wFlags() const noexcept {
        return static_cast<uint16_t>(det | (fInclude << 8) | (fFree << 9) | (ishdef << 10) | (fGift << 15));
    }
    uint8_t ihuldef; /* +0x0002 (1) */
    uint8_t ibmp;    /* +0x0003 (1) */
    uint16_t dp; /* Full armor / partial empty mass, decoded explicitly. */
    constexpr uint16_t wtEmpty() const noexcept { return dp; }
    uint8_t  chs;     /* +0x0006 (1) */
    uint16_t turn;    /* +0x0007 (2) */
    uint32_t cBuilt;  /* +0x0009 (4) */
    uint32_t cExist;  /* +0x000D (4) */
}; /* size=0x11 */

struct _rtchgshdef {
    uint16_t mdChg : 4, /* +0x0000 (2) @bit0 */
        iPlr : 4,       /* @bit4 */
        ishdef : 5,     /* @bit8 */
        junk : 3;       /* @bit13 */
    RTSHDEF rtshdef;    /* +0x0002 (17) */
}; /* size=0x13 */

// rtFleetA/rtFleetB: the first 12 bytes of FLEET, then the ship counts named
// by grbitCsh (bytes or words, per fByteCsh) and further packed fields.
struct _rtfleet {
    int16_t id;
    constexpr uint16_t ifl() const noexcept { return static_cast<uint16_t>((static_cast<uint32_t>(id) >> 0) & 0x1ff); }
    constexpr uint16_t SetIfl(uint16_t iflNew) noexcept {
        id = static_cast<int16_t>((static_cast<uint32_t>(id) & 0xfe00u) | ((static_cast<uint32_t>(static_cast<uint16_t>(iflNew)) & 0x1ffu) << 0));
        return ifl();
    }
    constexpr uint16_t iplr() const noexcept { return static_cast<uint16_t>((static_cast<uint32_t>(id) >> 9) & 0xf); }
    constexpr uint16_t SetIplr(uint16_t iplrNew) noexcept {
        id = static_cast<int16_t>((static_cast<uint32_t>(id) & 0xe1ffu) | ((static_cast<uint32_t>(static_cast<uint16_t>(iplrNew)) & 0xfu) << 9));
        return iplr();
    }
    constexpr uint16_t junk() const noexcept { return static_cast<uint16_t>((static_cast<uint32_t>(id) >> 13) & 0x7); }
    constexpr uint16_t SetJunk(uint16_t junkNew) noexcept {
        id = static_cast<int16_t>((static_cast<uint32_t>(id) & 0x1fffu) | ((static_cast<uint32_t>(static_cast<uint16_t>(junkNew)) & 0x7u) << 13));
        return junk();
    }
    int16_t  iPlayer;     /* +0x0002 (2) */
    uint16_t det : 8,     /* +0x0004 (2) @bit0 */
        fInclude : 1,     /* @bit8 */
        fRepOrders : 1,   /* @bit9 */
        fDead : 1,        /* @bit10 */
        fByteCsh : 1,     /* @bit11, FLEET.fDone in memory */
        fBombed : 1,      /* @bit12 */
        fHereAllTurn : 1, /* @bit13 */
        fNoHeal : 1,      /* @bit14 */
        fMark : 1;        /* @bit15 */
    int16_t  idPlanet;    /* +0x0006 (2) */
    POINT16  pt;          /* +0x0008 (4) */
    uint16_t grbitCsh;    /* +0x000C (2) */
}; /* size=0xe */

// rtLogFleetSplit holds the fleet split from; rtLogFleetMerge holds the fleet
// merged into, then the fleets merged into it.
// The borrowed bytes have a zero-byte wire prefix. This runtime view's size
// is never a record length, and no int16_t array is invented over wire storage.
struct alignas(alignof(std::span<const std::byte>)) _rtfleetids {
    std::span<const std::byte> rgid;
};

// MSGPLR as written to disk: the Win16 lpmsgplrNext far pointer is written as
// zero and ignored on read.
struct _rtplrmsg {
    uint32_t lpmsgplrNext; /* +0x0000 (4) */
    int16_t  iPlrFrom;     /* +0x0004 (2) */
    int16_t  iPlrTo;       /* +0x0006 (2) */
    int16_t  iInRe;        /* +0x0008 (2) */
    int16_t  cLen;         /* +0x000A (2) */
}; /* size=0xc */

struct _rtresearch {
    int8_t pctResearch;
    int8_t iTechCur;
    constexpr uint8_t iTechNow() const noexcept { return static_cast<uint8_t>(iTechCur) & 15; }
    constexpr uint8_t iTechNext() const noexcept { return static_cast<uint8_t>(iTechCur) >> 4; }
}; /* size=0x2 */

struct _rtshipint {
    int16_t id; /* +0x0000 (2) */
    int16_t i;  /* +0x0002 (2) */
}; /* size=0x4 */

struct _rtshipint2 {
    int16_t id; /* +0x0000 (2) */
    int16_t i;  /* +0x0002 (2) */
    int16_t i2; /* +0x0004 (2) */
}; /* size=0x6 */

struct _rtxfer {
    uint16_t id1;        /* +0x0000 (2) */
    uint16_t id2;        /* +0x0002 (2) */
    uint8_t  grobj1 : 4, /* +0x0004 (1) @bit0 */
        grobj2 : 4;      /* @bit4 */
    uint8_t grbitItems;  /* +0x0005 (1) */
    int8_t  rgcQuan[1];  /* +0x0006 (1) */
}; /* size=0x7 */

struct _rtxferf {
    uint16_t id1;        /* +0x0000 (2) */
    uint16_t id2;        /* +0x0002 (2) */
    uint8_t  grobj1 : 4, /* +0x0004 (1) @bit0 */
        grobj2 : 4;      /* @bit4 */
    uint16_t grbitItems; /* +0x0005 (2) */
    int16_t  rgcQuan[1]; /* +0x0007 (2) */
}; /* size=0x9 */

struct _rtxferl {
    uint16_t id1;        /* +0x0000 (2) */
    uint16_t id2;        /* +0x0002 (2) */
    uint8_t  grobj1 : 4, /* +0x0004 (1) @bit0 */
        grobj2 : 4;      /* @bit4 */
    uint8_t grbitItems;  /* +0x0005 (1) */
    int32_t rgcQuan[1];  /* +0x0006 (4) */
}; /* size=0xa */

struct _rtxferx {
    uint16_t id1;        /* +0x0000 (2) */
    uint16_t id2;        /* +0x0002 (2) */
    uint8_t  grobj1 : 4, /* +0x0004 (1) @bit0 */
        grobj2 : 4;      /* @bit4 */
    uint8_t grbitItems;  /* +0x0005 (1) */
    int16_t rgcQuan[1];  /* +0x0006 (2) */
}; /* size=0x8 */

struct _sbar {
    int16_t grbit; /* +0x0000 (2) */
    int16_t id;    /* +0x0002 (2) */
    POINT16 pt;    /* +0x0004 (4) */
    char   *psz;   /* +0x0008 (2) */
    SCAN   *pscan; /* +0x000A (2) */
}; /* size=0xc */

struct _scan {
    POINT16    pt;        /* +0x0000 (4) */
    GrobjClass grobj;     /* +0x0004 (2) */
    GrobjClass grobjFull; /* +0x0006 (2) */
    int16_t    idpl;      /* +0x0008 (2) */
    int16_t    ifl;       /* +0x000A (2) */
    int16_t    iwp;       /* +0x000C (2) */
    int16_t    ith;       /* +0x000E (2) */
}; /* size=0x10 */

struct _scanner {
    int16_t        id;             /* +0x0000 (2) */
    int8_t         rgTech[6];      /* +0x0002 (6) */
    char           szName[32];     /* +0x0008 (32) */
    int16_t        cMass;          /* +0x0028 (2) */
    uint16_t       resCost;        /* +0x002A (2) */
    int16_t        rgwtOreCost[3]; /* +0x002C (6) */
    int16_t        ibmp;           /* +0x0032 (2) */
    int16_t        dRange;         /* +0x0034 (2) */
    ScannerAbility grfAbilities;   /* +0x0036 (2) */
}; /* size=0x38 */

struct _score {
    int32_t  lScore;      /* +0x0000 (4) */
    int32_t  cResources;  /* +0x0004 (4) */
    int16_t  cPlanet;     /* +0x0008 (2) */
    int16_t  cStarbase;   /* +0x000A (2) */
    uint16_t rgcsh[3];    /* +0x000C (6) */
    int16_t  cTechLevels; /* +0x0012 (2) */
}; /* size=0x14 */

struct _scorex {
    uint16_t iPlayer : 5, /* +0x0000 (2) @bit0 */
        fValid : 1,       /* @bit5 */
        grbitVC : 8,      /* @bit6 */
        fWinner : 1,      /* @bit14 */
        fHistory : 1;     /* @bit15 */
    uint16_t wWord() const noexcept {
        return static_cast<uint16_t>(iPlayer | (fValid << 5) | (grbitVC << 6) | (fWinner << 14) | (fHistory << 15));
    }
    uint16_t turn; /* +0x0002 (2) */
    int16_t iRank() const noexcept { return std::bit_cast<int16_t>(uint16_t{turn}); }
    void SetIRank(int16_t iRank) noexcept { turn = std::bit_cast<uint16_t>(iRank); }
    SCORE score; /* +0x0004 (20) */
}; /* size=0x18 */

struct _selSome {
    POINT16    pt;        /* +0x0000 (4) */
    int16_t    grobj;     /* +0x0004 (2) */
    GrobjClass grobjFull; /* +0x0006 (2) */
    int16_t    id;        /* +0x0008 (2) */
    int16_t    iwpAct;    /* +0x000A (2) */
    SCAN       scan;      /* +0x000C (16) */
}; /* size=0x1c */

struct _shdef {
    HUL hul; /* +0x0000 (123) */
    uint16_t det : 8, /* Detail byte; encoded independently of native bitfield layout. */
        fInclude : 1,
        fFree : 1,
        ishdef : 5,
        fGift : 1;
    constexpr uint16_t wFlags() const noexcept {
        return static_cast<uint16_t>(det | (fInclude << 8) | (fFree << 9) | (ishdef << 10) | (fGift << 15));
    }
    uint16_t turn;   /* +0x007D (2) */
    uint32_t cBuilt; /* +0x007F (4) */
    uint32_t cExist; /* +0x0083 (4) */
    int32_t lPower; /* Combat power or squared starbase visibility. */
    int32_t lVisible() const noexcept { return lPower; }
    void SetLVisible(int32_t lVisibleNew) noexcept { lPower = lVisibleNew; }
    uint16_t grbitPlr;    /* +0x008B (2) */
    uint16_t dScanRange;  /* +0x008D (2) */
    uint16_t dScanRange2; /* +0x008F (2) */
    uint8_t  pctDetect;   /* +0x0091 (1) */
    uint8_t  iSteal;      /* +0x0092 (1) */
}; /* size=0x93 */

struct _shield {
    int16_t  id;             /* +0x0000 (2) */
    int8_t   rgTech[6];      /* +0x0002 (6) */
    char     szName[32];     /* +0x0008 (32) */
    int16_t  cMass;          /* +0x0028 (2) */
    uint16_t resCost;        /* +0x002A (2) */
    int16_t  rgwtOreCost[3]; /* +0x002C (6) */
    int16_t  ibmp;           /* +0x0032 (2) */
    int16_t  dp;             /* +0x0034 (2) */
}; /* size=0x36 */

struct _special {
    int16_t  id;             /* +0x0000 (2) */
    int8_t   rgTech[6];      /* +0x0002 (6) */
    char     szName[32];     /* +0x0008 (32) */
    int16_t  cMass;          /* +0x0028 (2) */
    uint16_t resCost;        /* +0x002A (2) */
    int16_t  rgwtOreCost[3]; /* +0x002C (6) */
    int16_t  ibmp;           /* +0x0032 (2) */
    int16_t  grAbility;      /* +0x0034 (2) */
}; /* size=0x36 */

struct _specialsb {
    int16_t  id;             /* +0x0000 (2) */
    int8_t   rgTech[6];      /* +0x0002 (6) */
    char     szName[32];     /* +0x0008 (32) */
    int16_t  cMass;          /* +0x0028 (2) */
    uint16_t resCost;        /* +0x002A (2) */
    int16_t  rgwtOreCost[3]; /* +0x002C (6) */
    int16_t  ibmp;           /* +0x0032 (2) */
    int16_t  grAbility;      /* +0x0034 (2) */
    int16_t  grAbility2;     /* +0x0036 (2) */
}; /* size=0x38 */

struct _starpack {
    uint32_t dx : 10, /* +0x0000 (4) @bit0 */
        y : 12,       /* @bit10 */
        id : 10;      /* @bit22 */
}; /* size=0x4 */

struct _tasklaymines {
    uint16_t cTime;    /* +0x0000 (2) */
    uint16_t cTimeOld; /* +0x0002 (2) */
}; /* size=0x4 */

struct _taskpatrol {
    uint16_t iWarp; /* +0x0000 (2) */
    uint16_t iDist; /* +0x0002 (2) */
}; /* size=0x4 */

struct _tasksell {
    uint16_t iPlrX; /* +0x0000 (2) */
}; /* size=0x2 */

struct alignas(2) _taskxport {
    ITEMACTION rgia[5]; /* +0x0000 (10) */
}; /* size=0xa */

struct alignas(2) _order {
    POINT16  pt;          /* +0x0000 (4) */
    int16_t  id;          /* +0x0004 (2) */
    uint16_t grTask : 4,  /* +0x0006 (2) @bit0 */
        iWarp : 4,        /* @bit4 */
        grobj : 4,        /* @bit8 */
        fValidTask : 1,   /* @bit12 */
        fNoAutoTrack : 1, /* @bit13 */
        fUnused : 2;      /* @bit14 */
    TASKXPORT txp;

    NATIVETASKLAYMINESVIEW<uint16_t> tlm() noexcept { return {txp.rgia[0].w, txp.rgia[1].w}; }
    NATIVETASKLAYMINESVIEW<const uint16_t> tlm() const noexcept { return {txp.rgia[0].w, txp.rgia[1].w}; }
    NATIVETASKPATROLVIEW<uint16_t> tptl() noexcept { return {txp.rgia[0].w, txp.rgia[1].w}; }
    NATIVETASKPATROLVIEW<const uint16_t> tptl() const noexcept { return {txp.rgia[0].w, txp.rgia[1].w}; }
    NATIVETASKSELLVIEW<uint16_t> tsell() noexcept { return {txp.rgia[0].w}; }
    NATIVETASKSELLVIEW<const uint16_t> tsell() const noexcept { return {txp.rgia[0].w}; }
}; /* size=0x12 */

struct alignas(2) PLORD {
    uint16_t cbItem : 8, /* +0x0000 (2) @bit0 */
        fMark : 1,       /* @bit8 */
        ht : 3,          /* @bit9 */
        cAlloc : 4;      /* @bit12 */
    uint8_t iordMax;     /* +0x0002 (1) */
    uint8_t iordMac;     /* +0x0003 (1) */
    ORDER *rgord() noexcept { return reinterpret_cast<ORDER *>(reinterpret_cast<uint8_t *>(this) + 4); }
    const ORDER *rgord() const noexcept { return reinterpret_cast<const ORDER *>(reinterpret_cast<const uint8_t *>(this) + 4); }
}; /* size=0x4 */

struct alignas(2) _rtwaypt {
    int16_t id;     /* +0x0000 (2) */
    int16_t iWaypt; /* +0x0002 (2) */
    ORDER   order;  /* +0x0004 (18) */
}; /* size=0x16 */

struct _terra {
    int16_t  id;             /* +0x0000 (2) */
    int8_t   rgTech[6];      /* +0x0002 (6) */
    char     szName[32];     /* +0x0008 (32) */
    int16_t  cMass;          /* +0x0028 (2) */
    uint16_t resCost;        /* +0x002A (2) */
    int16_t  rgwtOreCost[3]; /* +0x002C (6) */
    int16_t  ibmp;           /* +0x0032 (2) */
    int16_t  grAbility;      /* +0x0034 (2) */
}; /* size=0x36 */

struct _thmine {
    int32_t  cMines;      /* +0x0000 (4) */
    uint16_t grbitPlr;    /* +0x0004 (2) */
    uint8_t  iType;       /* +0x0006 (1) */
    uint8_t  fDetonate;   /* +0x0007 (1) */
    uint16_t grbitPlrNow; /* +0x0008 (2) */
}; /* size=0xa */

struct _thpack {
    uint16_t idPlanet : 10, /* +0x0000 (2) @bit0 */
        iWarp : 4,          /* @bit10 */
        fMoved : 1,         /* @bit14 */
        fInclude : 1;       /* @bit15 */
    int16_t  rgwtMin[3];    /* +0x0002 (6) */
    uint16_t wtMax : 14,    /* +0x0008 (2) @bit0 */
        iDecayRate : 2;     /* @bit14 */
}; /* size=0xa */

struct _thtrader {
    POINT16  ptDest;         /* +0x0000 (4) */
    uint16_t iWarp : 4,      /* +0x0004 (2) @bit0 */
        fInclude : 1,        /* @bit4 */
        unused : 11;         /* @bit5 */
    uint16_t    grbitPlr;    /* +0x0006 (2) */
    GrbitTrader grbitTrader; /* +0x0008 (2) */
}; /* size=0xa */

struct _thworm {
    uint16_t iStable : 2,  /* +0x0000 (2) @bit0 */
        cLastMove : 10,    /* @bit2 */
        fDestKnown : 1,    /* @bit12 */
        fInclude : 1;      /* @bit13 */
    uint16_t grbitPlr;     /* +0x0002 (2) */
    uint16_t grbitPlrTrav; /* +0x0004 (2) */
    uint16_t idPartner;    /* +0x0006 (2) */
}; /* size=0x8 */

struct _thing {
    // Actual byte storage, not a native overlay. Partial/neighbor file copies
    // retain their physical eighteen-byte image and every opaque payload bit.
    std::byte rgbThing[18];
    std::span<std::byte, 18> RgbRecord() noexcept { return rgbThing; }
    std::span<const std::byte, 18> RgbRecord() const noexcept { return rgbThing; }
private:
    std::span<std::byte, 18> RgbBorrowed() const noexcept { return std::span<std::byte, 18>(const_cast<std::byte *>(rgbThing), 18); }
public:
    uint16_t idFull() const noexcept { return NATIVETHINGHEADERVIEW<false>(RgbBorrowed()).idFull(); }
    void SetIdFull(uint16_t value) noexcept { NATIVETHINGHEADERVIEW<true>(RgbRecord()).SetIdFull(value); }
    uint16_t id() const noexcept { return NATIVETHINGHEADERVIEW<false>(RgbBorrowed()).id(); }
    void SetId(uint16_t value) noexcept { NATIVETHINGHEADERVIEW<true>(RgbRecord()).SetId(value); }
    uint16_t iplr() const noexcept { return NATIVETHINGHEADERVIEW<false>(RgbBorrowed()).iplr(); }
    void SetIplr(uint16_t value) noexcept { NATIVETHINGHEADERVIEW<true>(RgbRecord()).SetIplr(value); }
    uint16_t ith() const noexcept { return NATIVETHINGHEADERVIEW<false>(RgbBorrowed()).ith(); }
    void SetIth(uint16_t value) noexcept { NATIVETHINGHEADERVIEW<true>(RgbRecord()).SetIth(value); }
    uint16_t turn() const noexcept { return NATIVETHINGHEADERVIEW<false>(RgbBorrowed()).turn(); }
    void SetTurn(uint16_t value) noexcept { NATIVETHINGHEADERVIEW<true>(RgbRecord()).SetTurn(value); }
    NATIVETHINGPOINTVIEW<true> pt() noexcept { return {RgbRecord(), 2}; }
    NATIVETHINGPOINTVIEW<false> pt() const noexcept { return {RgbBorrowed(), 2}; }
    std::span<std::byte, 10> rgb() noexcept { return RgbRecord().subspan<6, 10>(); }
    std::span<const std::byte, 10> rgb() const noexcept { return RgbRecord().subspan<6, 10>(); }
    NATIVETHINGMINEVIEW<true> thm() noexcept { return {RgbRecord(), 6}; }
    NATIVETHINGMINEVIEW<false> thm() const noexcept { return {RgbBorrowed(), 6}; }
    NATIVETHINGPACKVIEW<true> thp() noexcept { return {RgbRecord(), 6}; }
    NATIVETHINGPACKVIEW<false> thp() const noexcept { return {RgbBorrowed(), 6}; }
    NATIVETHINGWORMVIEW<true> thw() noexcept { return {RgbRecord(), 6}; }
    NATIVETHINGWORMVIEW<false> thw() const noexcept { return {RgbBorrowed(), 6}; }
    NATIVETHINGTRADERVIEW<true> tht() noexcept { return {RgbRecord(), 6}; }
    NATIVETHINGTRADERVIEW<false> tht() const noexcept { return {RgbBorrowed(), 6}; }
};
static_assert(sizeof(THING) == 18 && alignof(THING) == 1); /* size=0x12 */

struct _sel {
    POINT16    pt;        /* +0x0000 (4) */
    GrobjClass grobj;     /* +0x0004 (2) */
    GrobjClass grobjFull; /* +0x0006 (2) */
    int16_t    id;        /* +0x0008 (2) */
    int16_t    iwpAct;    /* +0x000A (2) */
    SCAN       scan;      /* +0x000C (16) */
    FLEET      fl;        /* +0x001C (124) */
    PLANET     pl;        /* +0x0098 (56) */
    THING      th;        /* +0x00D0 (18) */
}; /* size=0xe2 */

/* NATIVE: tiles are UI-only, never serialized. Callback pointers and array
 * strides need native alignment instead of the surrounding Win16 packing. */
#pragma pack(push, 8)
struct _tile {
    int16_t      yTop;         /* +0x0000 (2) */
    int16_t      dyFull;       /* +0x0002 (2) */
    TileBits     grbit;        /* +0x0004 (2) */
    TILEDRAWPROC pfn;          /* +0x0006 (4) */
    uint16_t     iCol : 3,     /* +0x000A (2) @bit0 */
        id : 4,                /* @bit3 */
        fPopped : 1,           /* @bit7 */
        fNullPtr : 1,          /* @bit8 */
        fMinTitle : 1,         /* @bit9 */
        fErase : 1,            /* @bit10 */
        fFixCtls : 1,          /* @bit11 */
        fMinDraw : 1;          /* @bit12 */
    uint16_t      fUnused : 4; /* +0x000C (2) @bit0 */
    HelpContextId idh;         /* +0x000E (2) */
}; /* Win16 size=0x10; native alignment and pointer width determine size. */
#pragma pack(pop)

struct _timer {
    int16_t mdForce;        /* +0x0000 (2) */
    int16_t fAutoGenWhenIn; /* +0x0002 (2) */
    int16_t hrsForce; /* +0x0004 (2) */
    uint16_t minForce() const noexcept { return static_cast<uint16_t>(hrsForce) & 4095; }
    uint16_t cPlr() const noexcept { return static_cast<uint16_t>(hrsForce) >> 12; }
    void SetMinForce(uint16_t minForce) noexcept {
        const uint16_t w = static_cast<uint16_t>((static_cast<uint16_t>(hrsForce) & 0xf000u) | (minForce & 4095));
        hrsForce = std::bit_cast<int16_t>(w);
    }
    void SetCPlr(uint16_t cPlr) noexcept {
        const uint16_t w = static_cast<uint16_t>((static_cast<uint16_t>(hrsForce) & 4095u) | ((cPlr & 15) << 12));
        hrsForce = std::bit_cast<int16_t>(w);
    }
    int32_t tickcount; /* +0x0006 (4) */
}; /* size=0xa */

struct _tok {
    uint16_t id;            /* +0x0000 (2) */
    uint8_t  iplr;          /* +0x0002 (1) */
    uint8_t  grobj;         /* +0x0003 (1) */
    uint8_t  ishdef;        /* +0x0004 (1) */
    uint8_t  brc;           /* +0x0005 (1) */
    uint8_t  initBase;      /* +0x0006 (1) */
    uint8_t  initMin;       /* +0x0007 (1) */
    uint8_t  initMac;       /* +0x0008 (1) */
    uint8_t  itokTarget;    /* +0x0009 (1) */
    uint8_t  pctCloak;      /* +0x000A (1) */
    uint8_t  pctJam;        /* +0x000B (1) */
    uint8_t  pctBC;         /* +0x000C (1) */
    uint8_t  pctCap;        /* +0x000D (1) */
    uint8_t  pctBeamDef;    /* +0x000E (1) */
    uint16_t wt;            /* +0x000F (2) */
    uint16_t dpShield;      /* +0x0011 (2) */
    uint16_t csh;           /* +0x0013 (2) */
    DV       dv;            /* +0x0015 (2) */
    uint16_t mdTarget1 : 4, /* +0x0017 (2) @bit0 */
        mdTarget2 : 4,      /* @bit4 */
        mdTactic : 4,       /* @bit8 */
        mdTarget0 : 4;      /* @bit12 */
    uint16_t dxyLim : 4,    /* +0x0019 (2) @bit0 */
        dxyMax : 4,         /* @bit4 */
        spd : 4,            /* @bit8 */
        cTarget : 4;        /* @bit12 */
    uint16_t fActive : 1,
        fDetector : 1,
        fTorp : 1,
        fRegen : 1,
        fMoved : 1,
        dzDis : 5,
        dwt : 4,
        dMovesLeft : 2;
    constexpr uint16_t wFlags() const noexcept {
        return static_cast<uint16_t>(fActive | (fDetector << 1) | (fTorp << 2) | (fRegen << 3) |
                                     (fMoved << 4) | (dzDis << 5) | (dwt << 10) | (dMovesLeft << 14));
    }
}; /* size=0x1d */

struct _btldata {
    uint16_t id;       /* +0x0000 (2) */
    uint8_t  cplr;     /* +0x0002 (1) */
    uint8_t  ctok;     /* +0x0003 (1) */
    uint16_t grfPlr;   /* +0x0004 (2) */
    uint16_t cbData;   /* +0x0006 (2) */
    uint16_t idPlanet; /* +0x0008 (2) */
    POINT16  pt;       /* +0x000A (4) */
}; /* size=0xe */

struct _torp {
    int16_t  id;             /* +0x0000 (2) */
    int8_t   rgTech[6];      /* +0x0002 (6) */
    char     szName[32];     /* +0x0008 (32) */
    int16_t  cMass;          /* +0x0028 (2) */
    uint16_t resCost;        /* +0x002A (2) */
    int16_t  rgwtOreCost[3]; /* +0x002C (6) */
    int16_t  ibmp;           /* +0x0032 (2) */
    int16_t  dRangeMax;      /* +0x0034 (2) */
    int16_t  dp;             /* +0x0036 (2) */
    int16_t  init;           /* +0x0038 (2) */
    int16_t  dHitChance;     /* +0x003A (2) */
}; /* size=0x3c */

struct _vers {
    uint16_t verInc : 5, /* +0x0000 (2) @bit0 */
        verMinor : 7,    /* @bit5 */
        verMajor : 4;    /* @bit12 */
}; /* size=0x2 */

struct _wn {
    RECT     rc;             /* +0x0000 (8) */
    uint16_t fMaximized : 1, /* +0x0008 (2) @bit0 */
        fMinimized : 1,      /* @bit1 */
        fInitalized : 1,     /* @bit2 */
        fUnused : 13;        /* @bit3 */
}; /* size=0xa */

struct _ini {
    WN wnFrame; /* +0x0000 (10) */
    uint16_t fStartupFile : 1, /* +0x000A (2) @bit0 */
        fCmdLine : 1,          /* @bit1 */
        fWait : 1,             /* @bit2 */
        fGen : 1,              /* @bit3 */
        fTry : 1,              /* @bit4 */
        grobjSel : 4,          /* @bit5 */
        fBatch : 1,            /* @bit9 */
        fNewGame : 1,          /* @bit10 */
        fDumpFleets : 1,       /* @bit11 */
        fDumpPlanets : 1,      /* @bit12 */
        fDumpMap : 1,          /* @bit13 */
        fValidate : 1,         /* @bit14 */
        fLogging : 1;          /* @bit15 */
    uint16_t wFlags() const noexcept {
        const uint16_t w = uint16_t(fStartupFile) | uint16_t(fCmdLine) << 1 | uint16_t(fWait) << 2 | uint16_t(fGen) << 3 |
            uint16_t(fTry) << 4 | uint16_t(grobjSel) << 5 | uint16_t(fBatch) << 9 | uint16_t(fNewGame) << 10 |
            uint16_t(fDumpFleets) << 11 | uint16_t(fDumpPlanets) << 12 | uint16_t(fDumpMap) << 13 | uint16_t(fValidate) << 14 |
            uint16_t(fLogging) << 15;
        return w;
    }
    void SetWFlags(uint16_t wFlags) noexcept {
        const uint16_t w = wFlags;
        fStartupFile = w & 1u;
        fCmdLine = (w >> 1) & 1u;
        fWait = (w >> 2) & 1u;
        fGen = (w >> 3) & 1u;
        fTry = (w >> 4) & 1u;
        grobjSel = (w >> 5) & 15u;
        fBatch = (w >> 9) & 1u;
        fNewGame = (w >> 10) & 1u;
        fDumpFleets = (w >> 11) & 1u;
        fDumpPlanets = (w >> 12) & 1u;
        fDumpMap = (w >> 13) & 1u;
        fValidate = (w >> 14) & 1u;
        fLogging = (w >> 15) & 1u;
    }
    uint16_t turn;     /* +0x000C (2) */
    int16_t  iObjSel;  /* +0x000E (2) */
    int16_t  idPlayer; /* +0x0010 (2) */
    int32_t  lid;      /* +0x0012 (4) */
    int16_t  cTurnGen; /* +0x0016 (2) */
    int16_t  iMsg;     /* +0x0018 (2) */
}; /* size=0x1a */

// Runtime scratch follows its native payload alignment when root packing is
// retained for private allocation geometry. No file codec consumes this storage.
inline constexpr std::size_t cbXferPayload = (std::max)({sizeof(FLEET), sizeof(PLANET), sizeof(THING)});
inline constexpr std::size_t cbXferAlign = (std::max)({alignof(FLEET), alignof(PLANET), alignof(THING)});
struct _xfer {
    int16_t id;
    GrobjClass grobj;
    NATIVEOBJECTSTORAGE<cbXferPayload, cbXferAlign> storage;
    FLEET &fl() noexcept { return storage.As<FLEET>(); }
    const FLEET &fl() const noexcept { return storage.As<FLEET>(); }
    PLANET &pl() noexcept { return storage.As<PLANET>(); }
    const PLANET &pl() const noexcept { return storage.As<PLANET>(); }
    THING &th() noexcept { return storage.As<THING>(); }
    const THING &th() const noexcept { return storage.As<THING>(); }
    int16_t idObj() const noexcept { return storage.ReadPrefix<int16_t>(0); }
    void SetIdObj(int16_t value) noexcept { storage.WritePrefix(0, value); }
    int16_t iPlayerObj() const noexcept { return storage.ReadPrefix<int16_t>(2); }
    void SetIPlayerObj(int16_t value) noexcept { storage.WritePrefix(2, value); }
    uint16_t detObj() const noexcept { return storage.ReadPrefix<uint8_t>(4); }
    // Lookup defaults to THING; cargo defaults to FLEET. Keep both old domains.
    void *LpvLookup(GrobjClass kind) noexcept {
        if (kind == grobjFleet) return &fl();
        if (kind == grobjPlanet) return &pl();
        return &th();
    }
    void *LpvCargo(GrobjClass kind) noexcept {
        if (kind == grobjPlanet || kind == grobjOther) return &pl();
        if (kind == grobjThing) return &th();
        return &fl();
    }
};
static_assert(std::is_trivially_copyable_v<XFER> && std::is_standard_layout_v<XFER>);
static_assert(alignof(XFER) >= cbXferAlign && offsetof(XFER, storage) % cbXferAlign == 0);

struct _xferfull {
    uint16_t id1;        /* +0x0000 (2) */
    uint16_t id2;        /* +0x0002 (2) */
    uint8_t  grobj1 : 4, /* +0x0004 (1) @bit0 */
        grobj2 : 4;      /* @bit4 */
    int32_t rgcQuan[5];  /* +0x0005 (20) */
}; /* size=0x19 */

struct alignas(2) _ziporder {
    TASKXPORT txp;        /* +0x0000 (10) */
    char      szName[13]; /* +0x000A (13) */
    uint8_t   fValid;     /* +0x0017 (1) */
}; /* size=0x18 */

struct _zipprodq1 {
    uint8_t fNoResearch; /* +0x0000 (1) */
    uint8_t cpq;         /* +0x0001 (1) */
    PRODQ1  rgpq[12];    /* +0x0002 (24) */
}; /* size=0x1a */

struct _player {
    int8_t   iPlayer;     /* +0x0000 (1) */
    int8_t   cShDef;      /* +0x0001 (1) */
    int16_t  cPlanet;     /* +0x0002 (2) */
    uint16_t cFleet : 12, /* +0x0004 (2) @bit0 */
        cshdefSB : 4;     /* @bit12 */
    uint16_t det : 3, /* +0x0006 (2) @bit0 */
        iPlrBmp : 5,  /* @bit3 */
        fInclude : 1, /* @bit8 */
        fAi : 1,      /* @bit9 */
        lvlAi : 3,    /* @bit10 */
        idAi : 3;     /* @bit13 */
    uint16_t wMdPlr() const noexcept {
        return static_cast<uint16_t>(det | (iPlrBmp << 3) | (fInclude << 8) | (fAi << 9) | (lvlAi << 10) | (idAi << 13));
    }
    void SetWMdPlr(uint16_t wMdPlr) noexcept {
        det = wMdPlr & 7; iPlrBmp = (wMdPlr >> 3) & 31; fInclude = (wMdPlr >> 8) & 1;
        fAi = (wMdPlr >> 9) & 1; lvlAi = (wMdPlr >> 10) & 7; idAi = wMdPlr >> 13;
    }
    uint16_t mdPlayer() const noexcept { return wMdPlr() >> 9; }
    uint16_t reserved() const noexcept { return wMdPlr() & 511; }
    int16_t  idPlanetHome;   /* +0x0008 (2) */
    uint16_t wScore;         /* +0x000A (2) */
    int32_t  lSalt;          /* +0x000C (4) */
    int8_t   rgEnvVar[3];    /* +0x0010 (3) */
    int8_t   rgEnvVarMin[3]; /* +0x0013 (3) */
    int8_t   rgEnvVarMax[3]; /* +0x0016 (3) */
    int8_t   pctIdealGrowth; /* +0x0019 (1) */
    int8_t   rgTech[6];      /* +0x001A (6) */
    uint32_t rgResSpent[6];  /* +0x0020 (24) */
    int8_t   pctResearch;    /* +0x0038 (1) */
    int8_t iTechCur; /* +0x0039 (1) */
    uint8_t iTechNow() const noexcept { return static_cast<uint8_t>(iTechCur) & 15; }
    // Keep the signed high nibble of the historical iTechCur >> 4.
    int8_t iTechNext() const noexcept {
        const int16_t iTech = static_cast<uint8_t>(iTechCur) >> 4;
        return static_cast<int8_t>(iTech < 8 ? iTech : iTech - 16);
    }
    void SetITechNow(uint8_t iTechNow) noexcept {
        iTechCur = std::bit_cast<int8_t>(uint8_t((static_cast<uint8_t>(iTechCur) & 0xf0u) | (iTechNow & 15)));
    }
    void SetITechNext(int8_t iTechNext) noexcept {
        iTechCur = std::bit_cast<int8_t>(uint8_t((static_cast<uint8_t>(iTechCur) & 15u) | ((static_cast<uint8_t>(iTechNext) & 15) << 4)));
    }
    int32_t       lResLastYear; /* +0x003A (4) */
    int8_t        rgAttr[16];   /* +0x003E (16) */
    RaceTraitBits grbitAttr;    /* +0x004E (4) */
    GrbitTrader   grbitTrader;  /* +0x0052 (2) */
    uint16_t fDead : 1, /* +0x0054 (2) @bit0 */
        fCrippled : 1,  /* @bit1 */
        fUnused : 1,    /* @bit2 */
        fLearned : 1,   /* @bit3 */
        fHacker : 1,    /* @bit4 */
        unused : 11;    /* @bit5 */
    uint16_t wFlags() const noexcept {
        return static_cast<uint16_t>(fDead | (fCrippled << 1) | (fUnused << 2) | (fLearned << 3) | (fHacker << 4) | (unused << 5));
    }
    void SetWFlags(uint16_t wFlags) noexcept {
        fDead = wFlags & 1; fCrippled = (wFlags >> 1) & 1; fUnused = (wFlags >> 2) & 1;
        fLearned = (wFlags >> 3) & 1; fHacker = (wFlags >> 4) & 1; unused = wFlags >> 5;
    }
    ZIPPRODQ1 zpq1;             /* +0x0056 (26) */
    int8_t    rgmdRelation[16]; /* +0x0070 (16) */
    char      szName[32];       /* +0x0080 (32) */
    char      szNames[32];      /* +0x00A0 (32) */
}; /* size=0xc0 */

struct _tutor {
    uint16_t fVisible : 1, /* +0x0000 (2) @bit0 */
        fGameSaved : 1,    /* @bit1 */
        fChange : 1,       /* @bit2 */
        fTurnDone : 1,     /* @bit3 */
        fTutorDone : 1,    /* @bit4 */
        fNoErrors : 1,     /* @bit5 */
        cError : 3,        /* @bit6 */
        fAutoComplete : 1, /* @bit9 */
        fProgress : 1,     /* @bit10 */
        fTBVis : 1,        /* @bit11 */
        fValidQ : 1,       /* @bit12 */
        fFreeing : 1,      /* @bit13 */
        fShowHidMsg : 1,   /* @bit14 */
        unused : 1;        /* @bit15 */
    int16_t wFlags() const noexcept {
        const uint16_t w = uint16_t(fVisible) | uint16_t(fGameSaved) << 1 | uint16_t(fChange) << 2 | uint16_t(fTurnDone) << 3 |
            uint16_t(fTutorDone) << 4 | uint16_t(fNoErrors) << 5 | uint16_t(cError) << 6 | uint16_t(fAutoComplete) << 9 |
            uint16_t(fProgress) << 10 | uint16_t(fTBVis) << 11 | uint16_t(fValidQ) << 12 | uint16_t(fFreeing) << 13 |
            uint16_t(fShowHidMsg) << 14 | uint16_t(unused) << 15;
        return std::bit_cast<int16_t>(w);
    }
    void SetWFlags(int16_t wFlags) noexcept {
        const uint16_t w = std::bit_cast<uint16_t>(wFlags);
        fVisible = w & 1u;
        fGameSaved = (w >> 1) & 1u;
        fChange = (w >> 2) & 1u;
        fTurnDone = (w >> 3) & 1u;
        fTutorDone = (w >> 4) & 1u;
        fNoErrors = (w >> 5) & 1u;
        cError = (w >> 6) & 7u;
        fAutoComplete = (w >> 9) & 1u;
        fProgress = (w >> 10) & 1u;
        fTBVis = (w >> 11) & 1u;
        fValidQ = (w >> 12) & 1u;
        fFreeing = (w >> 13) & 1u;
        fShowHidMsg = (w >> 14) & 1u;
        unused = (w >> 15) & 1u;
    }
    TutorId       idt;       /* +0x0002 (2) */
    TutorId       idtBold;   /* +0x0004 (2) */
    HelpContextId idh;       /* +0x0006 (2) */
    int16_t       idsError;  /* +0x0008 (2) */
    ScanZoom      iScanZoom; /* +0x000A (2) */
    int16_t       icolFSort; /* +0x000C (2) */
    uint16_t      grbitScan; /* +0x000E (2) */
    HWND          hwnd;      /* +0x0010 (2) */
    ZIPPRODQ1     zpq;       /* +0x0012 (26) */
}; /* size=0x2c */

struct _zipprodq {
    char    szName[13]; /* +0x0000 (13) */
    uint8_t fValid;     /* +0x000D (1) */
    uint8_t fNoResearch; /* +0x000E (1) */
    uint8_t cpq;         /* +0x000F (1) */
    PRODQ1  rgpq[12];    /* +0x0010 (24) */
    ZIPPRODQ1 zpq1() const noexcept {
        ZIPPRODQ1 zpq{};
        zpq.fNoResearch = fNoResearch; zpq.cpq = cpq;
        for (int i = 0; i < 12; i++) zpq.rgpq[i] = rgpq[i];
        return zpq;
    }
    void SetZpq1(const ZIPPRODQ1 &zpq1) noexcept {
        fNoResearch = zpq1.fNoResearch; cpq = zpq1.cpq;
        for (int i = 0; i < 12; i++) rgpq[i] = zpq1.rgpq[i];
    }
}; /* size=0x28 */

#ifndef STARS_TEST_NATURAL_RECORD_LAYOUT
#pragma pack(pop)
#endif

#include "partviews.h"

#endif
