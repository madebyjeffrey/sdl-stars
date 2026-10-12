#ifndef STARS_DECOMPILED_ENUMS_H
#define STARS_DECOMPILED_ENUMS_H

enum HeapType { htOrd = 0, htString, htMsg, htPlanets, htLog, htFleets, htMisc, htShips, htPlrMsg, htPerm, htThings, htBattle, htCount };
typedef uint16_t HeapType;

enum AiLevel {
    lvlAiEasy = 0,
    lvlAiStandard = 1,
    lvlAiTough = 2,
    lvlAiExpert = 3,
    lvlAiRandom = 4,
};
typedef uint16_t AiLevel;

enum DetType {
    detNone = 0,
    detMinimal = 1,
    detObscure = 2,
    detSome = 3,
    detMore = 4,
    detAll = 7,
};
typedef uint16_t DetType;

enum MineralType {
    SupplyButtonsOnly = -2, // transfer dialog: redraw only the buttons
    SupplyAll = -1,         // transfer dialog: redraw every cargo row
    Ironium = 0,
    Boranium = 1,
    Germanium = 2,
    Colonists = 3,
    Resources = 3,
    Fuel = 4,
};
typedef int16_t MineralType;

enum EnvType {
    Gravity = 0,
    Temperature = 1,
    Radiation = 2,
};
typedef uint16_t EnvType;

enum TechFieldType {
    Energy = 0,
    Weapons = 1,
    Propulsion = 2,
    Construction = 3,
    Electronics = 4,
    Biotechnology = 5,
    TechFieldCount = 6,
};
typedef uint16_t TechFieldType;

enum BattleUnitFlags {
    grBuOurUnits = 0x0001,
    grBuTheirUnits = 0x0002,
    grBuIncludeSb = 0x0004,
    // hull classes, by HullCategory as the battle report columns count them
    grBuClassUnarmed = 0x0008,
    grBuClassScout = 0x0010,
    grBuClassWarship = 0x0020,
    grBuClassBomber = 0x0040,
    grBuClassUtility = 0x0080,
    grBuClassAll = 0x00F8,
};
typedef uint16_t BattleUnitFlags;

enum AttackWho {
    iplrAttackNobody = 0,
    iplrAttackEnemies = 1,
    iplrAttackNeutralsEnemies = 2,
    iplrAttackEveryone = 3,
    iplrAttackPlayer = 4,
};
typedef uint16_t AttackWho;

enum mdProdStat {
    mdProdStatComplete = 0,
    mdProdStatCompleteAuto = 1,
    mdProdStatSkippedAuto = 2,
    mdProdStatSomeAuto = 3,
    mdProdStatNoneAuto = 4,
    mdProdStatSome = 5,
    mdProdStatBlockedDiff = 6,
    mdProdStatBlockedSame = 7,
};
typedef uint16_t mdProdStat;

enum mdPartAvail {
    mdPartAvailRestricted = -1,
    mdPartAvailInvalid = 0,
    mdPartAvailAvailable = 1,
    mdPartAvailNextTech = 2,
    mdPartAvailOtherTech = 99,
};
typedef int16_t mdPartAvail;

enum GrPopupType {
    grPopupMineral = 1,
    grPopupPlayer = 2,
    grPopupFleet = 3,
    grPopupUnknownObj = 4,
    grPopupPlanetEnv = 5,
    grPopupShipOrders = 6,
    grPopupPlanet = 7,
    grPopupPlanetIndustry = 8,
    grPopupComponent = 9,
    grPopupString = 10,
    grPopupShdef = 11,
    grPopupResources = 12,
    grPopupUnknown = 13,
    grPopupShdefSB = 14,
    grPopupShdefBuild = 15,
};
typedef uint16_t GrPopupType;

enum HtMineType {
    htMineNone = 0,
    htMineMineralConc1 = 1,
    htMineMineralConc2 = 2,
    htMineMineralConc3 = 3,
    htMineUnused4 = 4,
    htMineScale = 5,
    htMineEnvVar0 = 6,
    htMineEnvVar1 = 7,
    htMineEnvVar2 = 8,
    htMineScanSel = 9,
    htMineOwner = 10,
    htMineShipOrFleet = 11,
    htMinePlanet = 12,
    htMineStarbase = 13,
    htMineMinefieldType = 14,
};
typedef uint16_t HtMineType;

enum HtMsgType {
    htMsgNone = 0,
    htMsgCurrent = 1,
    htMsgZoom = 2,
    htMsgMode = 3,
};
typedef uint16_t HtMsgType;

enum DtFileType {
    dtXY = 0,
    dtLog = 1,
    dtHost = 2,
    dtTurn = 3,
    dtHist = 4,
    dtRace = 5,
};
typedef uint16_t DtFileType;

enum RaceGrbit {
    ibitRaceIFE = 0x00,
    ibitRaceTT = 0x01,
    ibitRaceARM = 0x02,
    ibitRaceISB = 0x03,
    ibitRaceGeneralizedResearch = 0x04,
    ibitRaceUltimateRecycling = 0x05,
    ibitRaceMineralAlchemy = 0x06,
    ibitRaceNoRamscoops = 0x07,
    ibitRaceCheapEngines = 0x08,
    ibitRaceOBRM = 0x09,
    ibitRaceNoAdvScanner = 0x0a,
    ibitRaceLowStartingPop = 0x0b,
    ibitRaceBleedingEdgeTech = 0x0c,
    ibitRaceRegeneratingShields = 0x0d,
    ibitRaceTech3 = 0x1d,
    ibitRaceAIPlayer = 0x1e,
    ibitRaceCheapFact = 0x1f,
    ibitRaceLast = 32,
};
typedef uint16_t RaceGrbit;

// RaceTraitBits is a race's grbitAttr: one bit per RaceGrbit index, the
// lesser racial traits and the other per-race switches.
enum RaceTraitBits {
    grbitRaceIFE = 0x00000001,
    grbitRaceTT = 0x00000002,
    grbitRaceARM = 0x00000004,
    grbitRaceISB = 0x00000008,
    grbitRaceGeneralizedResearch = 0x00000010,
    grbitRaceUltimateRecycling = 0x00000020,
    grbitRaceMineralAlchemy = 0x00000040,
    grbitRaceNoRamscoops = 0x00000080,
    grbitRaceCheapEngines = 0x00000100,
    grbitRaceOBRM = 0x00000200,
    grbitRaceNoAdvScanner = 0x00000400,
    grbitRaceLowStartingPop = 0x00000800,
    grbitRaceBleedingEdgeTech = 0x00001000,
    grbitRaceRegeneratingShields = 0x00002000,
    grbitRaceTech3 = 0x20000000,
    grbitRaceAIPlayer = 0x40000000,
    grbitRaceCheapFact = 0x80000000,
};
typedef uint32_t RaceTraitBits;

enum RaceStat {
    rsResGen = 0,
    rsFactProd = 1,
    rsFactBuild = 2,
    rsFactOperate = 3,
    rsMineProd = 4,
    rsMineBuild = 5,
    rsMineOperate = 6,
    rsUseLeftover = 7,
    rsTechBonus1 = 8,
    rsTechBonus2 = 9,
    rsTechBonus3 = 10,
    rsTechBonus4 = 11,
    rsTechBonus5 = 12,
    rsTechBonus6 = 13,
    rsMajorAdv = 14,
};
typedef uint16_t RaceStat;

enum RaceAttribute {
    raCheapCol = 0,
    raStealth = 1,
    raAttack = 2,
    raTerra = 3,
    raDefend = 4,
    raMines = 5,
    raMassAccel = 6,
    raStargate = 7,
    raMacintosh = 8,
    raNone = 9,
    raMax = 10,
};
typedef uint16_t RaceAttribute;

enum GrobjClass {
    grobjNone = 0x0,
    grobjPlanet = 0x1,
    grobjFleet = 0x2,
    grobjOther = 0x4,
    grobjThing = 0x8,
    mdNoRecurse = 0x0020,
    mdScanRadius = 0x0040,
    mdExact = 0x0080,
    mdRecurseMask = mdNoRecurse | mdExact,
};
typedef uint16_t GrobjClass;

enum HullSlotType {
    hstNone = 0x0000,
    hstEngine = 0x0001,
    hstScanner = 0x0002,
    hstShield = 0x0004,
    hstArmor = 0x0008,
    hstBeam = 0x0010,
    hstTorp = 0x0020,
    hstBomb = 0x0040,
    hstMining = 0x0080,
    hstMines = 0x0100,
    hstSpecialSB = 0x0200,
    hstSBHull = 0x0400,
    hstSpecialE = 0x0800,
    hstSpecialM = 0x1000,
    hstTerra = 0x2000,
    hstHull = 0x4000,
    hstPlanetary = 0x8000,
    hstWeapon = hstBeam | hstTorp,
    hstShArm = hstShield | hstArmor,
    hstSpecialEM = hstSpecialE | hstSpecialM,
    hstScanSpec = hstScanner | hstSpecialE | hstSpecialM,
    hstShWeap = hstShield | hstBeam | hstTorp,
    hstSomeSB = hstSpecialSB | hstSpecialE,
    hstSpecMine = hstSpecialEM | hstMines,
    hstShSpec = hstShield | hstSpecialE | hstSpecialM,
    hstScanSpecArm = hstScanner | hstArmor | hstSpecialE | hstSpecialM,
    hstEnabled = 0x19FF,
    hstEnabledSB = hstShield | hstArmor | hstWeapon | hstSpecialSB | hstSpecialE,
    hstSome = 0x193E,

};
typedef uint16_t HullSlotType;

// HulDef, StartingStarbase, StartingShip, the part indexes (isbhull,
// iengine ... ispecialSB) and the AI recipe offsets come from the YAML in
// data/; see data/datagen.py.
#include "dataids.h"

typedef uint16_t HulDef;
typedef uint16_t StartingStarbase;
typedef uint16_t StartingShip;


enum ThingType {
    ithMinefield = 0,
    ithMineralPacket = 1,
    ithWormhole = 2,
    ithMysteryTrader = 3,
};
typedef uint16_t ThingType;

enum MdBuild {
    mdBuildShdef = 0,
    mdBuildHuldef = 1,
    mdBuildEnemyShdef = 2,
    mdBuildComp = 3,
    mdBuildEdit = 4,
};
typedef uint16_t MdBuild;

enum MdXfer {
    mdXferNone = -1,
    mdXferCargo = 0,
    mdXferShips = 1,
};
typedef int16_t MdXfer;

enum ProdItemType {
    iobjMine = 0,
    iobjFactory = 1,
    iobjDefense = 2,
    iobjAlchemy = 3,
    iobjMinTerraform = 4,
    iobjMaxTerraform = 5,
    iobjPacket = 6,
    mdIdleFactory = 7,
    mdIdleMine = 8,
    mdIdleDefense = 9,
    /* 10 unused ? */
    mdIdleAlchemy = 11,
    mdIdleTerraform = 12,
    iobjGenesis = 13,
    iobjPacketIron = 14,
    iobjPacketBor = 15,
    iobjPacketGerm = 16,
    iobjPacketMixed = 17,

    iobjPlanetaryScannerFirst = 18,
    iobjPlanetaryScannerViewer50 = 18,
    iobjPlanetaryScannerViewer90 = 19,
    iobjPlanetaryScannerScoper150 = 20,
    iobjPlanetaryScannerScoper220 = 21,
    iobjPlanetaryScannerScoper280 = 22,
    iobjPlanetaryScannerSnooper320X = 23,
    iobjPlanetaryScannerSnooper400X = 24,
    iobjPlanetaryScannerSnooper500X = 25,
    iobjPlanetaryScannerSnooper620X = 26,
    iobjPlanetaryScannerLast = 26,
    iobjPlanetaryScanner = 27,
    iobjUnknown = 31,
};
typedef uint16_t ProdItemType;

enum PlayerSentinel {
    iplrNone = -1,
};
typedef int16_t PlayerSentinel;

enum FleetLocationSentinel {
    idPlanetDeepSpace = -1,
};
typedef int16_t FleetLocationSentinel;

enum PlanetIdSentinel {
    idplNone = -1,
};
typedef int16_t PlanetIdSentinel;

enum FleetIdSentinel {
    idflNone = -1,
};
typedef int16_t FleetIdSentinel;

enum ScanSelSentinel {
    iflNone = -1,
    iwpNone = -1,
    ithNone = -1,
};
typedef int16_t ScanSelSentinel;

enum LookupRequest {
    idWriteBack = -1,
};
typedef int16_t LookupRequest;

enum ShipDesignSentinel {
    ishdefNone = -1,
    ishdefAll = -1,
};
typedef int16_t ShipDesignSentinel;

enum MessageIndexSentinel {
    imsgNone = -1,
};
typedef int16_t MessageIndexSentinel;

enum EnvironmentValueSentinel {
    envImmune = -1,
    envNone = -1,
};
typedef int16_t EnvironmentValueSentinel;

enum ProdScheduleRequest {
    iprodEstimateResearchResources = 0xffff,
};
typedef uint16_t ProdScheduleRequest;

enum TutorCheckSentinel {
    iWarpAny = 0xffff,
    iDistAny = 0xffff,
};
typedef uint16_t TutorCheckSentinel;

enum TutorAnySentinel {
    mdScanAny = -1,
    iZoomAny = -1,
    iCategoryAny = -1,
    iShipAny = -1,
    idAny = -1,
    imsgAny = -1,
};
typedef int16_t TutorAnySentinel;

enum BattleRecordSentinel {
    idPlanetNone = 0xffff,
};
typedef uint16_t BattleRecordSentinel;

// StringId, MessageId and TutorId come from the text tables in text/; see
// text/textgen.py. TutorId is a tutorial text fragment, eight to a page;
// tutor.idt holds a page's first fragment and tutor.idtBold the highlighted
// instruction.
#include "textids.h"

enum StringIdSentinel {
    idsNoString = 0xffff,
};
typedef uint16_t StringId;
enum MessageIdSentinel {
    idmNone = 0xffff,
};
typedef uint16_t MessageId;
typedef uint16_t TutorId;

enum GrStat {
    grStatFuel = 1,
    grStatCargo = 2,

};
typedef uint16_t GrStat;

// isbhull is a starbase hull's index in rghuldefSB, its HulDef id less
// ihuldefOrbitalFort.
enum isbhullSentinel {
    isbhullAuto = -1, // FCreateAiStarbase: choose by design slot
};
typedef int16_t isbhull;

enum iengineSentinel {
    iengineNone = 0xffff,
};
typedef uint16_t iengine;
typedef uint16_t iarmor;
typedef uint16_t iscanner;
typedef uint16_t ishield;
typedef uint16_t ispecialE;
typedef uint16_t ispecialM;
typedef uint16_t imines;
typedef uint16_t imining;
typedef uint16_t iplanetary;
typedef uint16_t iterra;
typedef uint16_t ibomb;
typedef uint16_t itorp;
typedef uint16_t ibeam;
typedef uint16_t ispecialSB;

enum GrbitTrader {
    grbitTraderNone = 0x0000,
    grbitTraderCargo = 0x0001,
    grbitTraderSpecial = 0x0002,
    grbitTraderShield = 0x0004,
    grbitTraderArmor = 0x0008,
    grbitTraderMiner = 0x0010,
    grbitTraderBomb = 0x0020,
    grbitTraderTorp = 0x0040,
    grbitTraderBeam = 0x0080,
    grbitTraderHull = 0x0100,
    grbitTraderEngine = 0x0200,
    grbitTraderGenesis = 0x0400,
    grbitTraderJumpgate = 0x0800,
    grbitTraderLifeboat = 0x1000,
    grbitTraderAll = 0x1fff,
};
typedef uint16_t GrbitTrader;

enum LookupResult {
    LookupInvalid = 0,     // “out of range” / not a valid part id in group
    LookupDisallowed = -1, // disallowed for race/trait/other rule
    LookupOk = 1,          // meets tech reqs (original CheckTechRequirements == 1)
    LookupNear = 2,        // “one level away in current research field”
    LookupNeedMany = 99    // multiple tech deficits
};
typedef int16_t LookupResult;

enum RecordType {
    /*
     * NOTE: Stars! file records encode a 6-bit "record type" (rt) plus a 10-bit
     * byte count (cb) in a 16-bit header word.
     *
     * In .HST files (and others), record type 0x00 is used for the footer record
     * (cb=2, data=0000). The original code treats "rt==0" as a terminator while
     * reading, so we keep rtEOF=0 for that behavior.
     */
    rtEOF = 0,
    rtLogCargoXfer8 = 1,         /* quantities are int8  (lpb[6+iLook]) */
    rtLogCargoXfer16 = 2,        /* quantities are int16 (lpb[6+2*iLook]) */
    rtLogFleetOrderDelete = 3,   /* delete 1 or 2 orders; index in *(u16*)(lpb+2), high bit => delete extra */
    rtLogFleetOrderInsert = 4,   /* insert new order at index *(i16*)(lpb+2); payload from lpb+4 */
    rtLogFleetOrderUpdate = 5,   /* overwrite existing order at index *(i16*)(lpb+2); payload from lpb+4 */
    rtPlr = 6,                   /* Player */
    rtGame = 7,                  /* Game */
    rtBOF = 8,                   /* FileHeader / BOF */
    rtLogFleetFlagBit9 = 10,     /* lpfl->wFlags_0x4 bit 9 set/cleared by (*(u16*)(lpb+2) & 1) */
    rtLogFleetOrderAttrNib = 11, /* order[index].word10 low nibble set to (*(i16*)(lpb+4) & 0xF), value constrained <=9 */
    rtMsg = 12,                  /* Message */
    rtPlanet = 13,
    rtPlanetB = 14,
    rtFleetA = 16,
    rtFleetB = 17,
    rtOrderA = 19, /* other order-like record type seen in decompile */
    rtOrderB = 20, // waypoint only
    rtString = 21, /* decompile: alloc/copy string from rgbCur when rt == 0x15 */
    rtSel = 22,    /* decompile: after things, if (rt == 0x16) ReadRt(); matches file.c rtSel */
    rtLogFleetCargoXfer = 23,
    rtLogFleetSplit = 24,  /* LpflNewSplit(&fleet) */
    rtLogCargoXfer32 = 25, /* quantities are int32 (lpb[6+4*iLook]) */
    rtShDef = 26,
    rtLogShDef = 27, /* Ship design definition (SHDEF) create/update/delete for the current player. */
    rtProdQ = 28,
    rtLogPlanetProdQ = 29,   /* Planet production queue set/clear (planet->lpplprod). */
    rtBtlPlan = 30,          /* decompile: while (rt == 0x1e) { ...battle plan... } */
    rtBtlData = 31,          /* decompile: while (rt == 0x1f || rt == 0x27) { ... } */
    rtContinue = 39,         /* decompile: inside loop: if (rt != 0x27) { ... } matches `rt != rtContinue` */
    rtHistHdr = 32,          /* decompile: after opening dtHist, expects rt == 0x20 */
    rtMsgFilt = 33,          /* decompile: checks cbbitfMsg vs cb and memcpy(bitfMsgFiltered, ...) */
    rtLogResearch = 34,      /* Research settings: pctResearch + iTechCur (packed nibble fields). */
    rtLogPlanetRouting = 35, /* Planet routing / starbase / infrastructure bitfields mutation. */
    rtLogRelations = 38,     /* memcpy rgplr[idPlayer].rgmdRelation[0..cPlayer) */
    rtChgPassword = 36,      /* file.c: if (hdrCur.rt == rtChgPassword) { lSaltCur = *(long*)rgbCur; } */
    rtLogFleetMerge = 37,    /* merge all-at-location (cb==2) or merge listed fleet ids (cb>2) */
    rtPlrMsg = 40,
    rtAiData = 41,            /* decompile: loop skips/reads while (rt == 0x29) around vlpbAiData */
    rtThing = 43,             /* decompile: if (rt == 0x2b) { cThing = rgbCur; alloc things } */
    rtLogFleetPlan = 42,      /* lpfl->iplan = *(u16*)(lpb+2) truncated */
    rtLogThingByteParam = 43, /* sets 1 byte inside THING union for a restricted subtype */
    rtLogFleetName = 44,      /* User string (fleet rename); may be compressed via FDecompressUserString. */
    rtScore = 45,             /* decompile: loop `if (rt != 0x2d) break;` in score load path */
    rtLogPlayerZpq1 = 46,     /* Host-only opaque blob (size capped at 0x1A bytes) copied into rgplr[idPlayer].zpq1. */
    rtMax = 47                /* one past highest observed (0x2d) */
};
typedef uint16_t RecordType;

enum DialogId {
    /* ship / fleet */
    IDD_MERGE_FLEETS = 82, /* MergeFleetsDlg */
    IDD_TRANSFER = 91,     /* TransferDlg */
    IDD_SLOT = 92,         /* SlotDlg */
    IDD_PRODUCTION = 93,   /* ProductionDlg */

    /* common / utility */
    IDD_ZIP_PROD = 89,      /* ZipProdDlg / ZipOrderDlg */
    IDD_ABOUT = 90,         /* About */
    IDD_HOST_MODE = 115,    /* HostOptionsDialog */
    IDD_PASSWORD = 140,     /* PASSWORD dialog */
    IDD_NEW_PASSWORD = 141, /* NewPasswordDlg */

    /* research / browser */
    IDD_RESEARCH = 127, /* ResearchDlg */
    IDD_BROWSER = 128,  /* Browser child dialog */

    /* race wizard */
    IDD_RACE_WIZARD_1 = 146, /* RaceWizardDlg1 */
    IDD_RACE_WIZARD_2 = 147, /* unnamed proc (slot between 1 and 3) */
    IDD_RACE_WIZARD_3 = 148, /* RaceWizardDlg3 */
    IDD_RACE_WIZARD_4 = 149, /* RaceWizardDlg4 */
    IDD_RACE_WIZARD_5 = 150, /* RaceWizardDlg5 */
    IDD_RACE_WIZARD_6 = 151, /* RaceWizardDlg6 */

    /* VCR */
    IDD_VCR = 160, /* VCRDlg */

    /* new game */
    IDD_SIMPLE_NEW_GAME = 209, /* SimpleNewGameDlg */
    IDD_NEW_GAME_1 = 390,      /* NewGameDlg */
    IDD_NEW_GAME_2 = 391,      /* NewGameDlg2 */
    IDD_NEW_GAME_3 = 392,      /* NewGameDlg3 */

    IDD_Gauge = 393, /* never seen invoked */

    /* host / options */
    IDD_HOST_OPTIONS = 1026, /* HostOptionsDialog */
    IDD_SCORE = 102,         /* ScoreXDlg */

    /* battle / plans */
    IDD_BATTLE_PLANS = 2013, /* BattlePlansDlg */
    IDD_RENAME = 2019,       /* RenameDlg / NewPlanNameDlg (shared template) */

    /* relations / diplomacy */
    IDD_RELATIONS = 2008, /* RelationsDlg */

    IDD_SAVE_TURN1 = 1068, /* Save  */
    IDD_SAVE_TURN2 = 2025, /* Save  */

    /* tutorial / panic */
    IDD_PANIC = 2504, /* PanicDlg */
    IDD_TUTOR = 2502, /* never referenced */

    /* find */
    IDD_FIND = 4202,     /* FindDlg */
    IDD_PRINT_MAP = 214, /* PrintMapDlg */
};
typedef uint16_t DialogId;

#undef IDOK
#undef IDCANCEL
#undef IDHELP
#undef IDC_HELP

// AboutControl names the controls of the About dialog.
enum AboutControl {
    IDC_ABOUT_ORDER_INFO = 118,
    IDC_ABOUT_DEMO_TEXT = 1025,
    IDC_ABOUT_CREDITS_TEXT = 1055,
};
typedef uint16_t AboutControl;

// PrintMapControl names the controls of the Print Map dialog.
enum PrintMapControl {
    IDC_PRINT_MAP_PAGES_X = 268,
    IDC_PRINT_MAP_PAGES_Y = 269,
};
typedef uint16_t PrintMapControl;

// SimpleNewGameControl names the controls of the simple New Game dialog.
enum SimpleNewGameControl {
    IDC_SIMPLE_NEW_GAME_EASY = 200,
    IDC_SIMPLE_NEW_GAME_STANDARD = 201,
    IDC_SIMPLE_NEW_GAME_HARDER = 202,
    IDC_SIMPLE_NEW_GAME_EXPERT = 203,
    IDC_SIMPLE_NEW_GAME_CUSTOMIZE_RACE = 210,
    IDC_SIMPLE_NEW_GAME_ADVANCED = 211,
    IDC_SIMPLE_NEW_GAME_TUTORIAL = 212,
    IDC_SIMPLE_NEW_GAME_TINY = 1000,
    IDC_SIMPLE_NEW_GAME_SMALL = 1001,
    IDC_SIMPLE_NEW_GAME_MEDIUM = 1002,
    IDC_SIMPLE_NEW_GAME_LARGE = 1003,
    IDC_SIMPLE_NEW_GAME_HUGE = 1004,
};
typedef uint16_t SimpleNewGameControl;

// NewGame1Control names the controls of the first advanced New Game wizard page.
enum NewGame1Control {
    IDC_NEW_GAME_TINY = 1000,
    IDC_NEW_GAME_SMALL = 1001,
    IDC_NEW_GAME_MEDIUM = 1002,
    IDC_NEW_GAME_LARGE = 1003,
    IDC_NEW_GAME_HUGE = 1004,
    IDC_NEW_GAME_SPARSE = 1005,
    IDC_NEW_GAME_NORMAL = 1006,
    IDC_NEW_GAME_DENSE = 1007,
    IDC_NEW_GAME_PACKED = 1008,
    IDC_NEW_GAME_CLOSE = 1009,
    IDC_NEW_GAME_MODERATE = 1010,
    IDC_NEW_GAME_FARTHER = 1011,
    IDC_NEW_GAME_DISTANT = 1012,
    IDC_NEW_GAME_MAX_MINERALS = 1016,
    IDC_NEW_GAME_SLOWER_TECH = 1017,
    IDC_NEW_GAME_ACCELERATED_BBS = 1018,
    IDC_NEW_GAME_NO_RANDOM_EVENTS = 1019,
    IDC_NEW_GAME_AI_ALLIANCES = 1020,
    IDC_NEW_GAME_PUBLIC_SCORES = 1021,
    IDC_NEW_GAME_NAME = 1030,
    IDC_NEW_GAME_GALAXY_CLUMPING = 1050,
};
typedef uint16_t NewGame1Control;

// NewGame3Control names the controls of the victory conditions New Game wizard page; the tech level checkbox covers both tech conditions.
enum NewGame3Control {
    IDC_VC_OWNS_PLANETS = 291,
    IDC_VC_TECH_LEVEL = 292,
    IDC_VC_SCORE = 293,
    IDC_VC_SECOND_PLACE = 294,
    IDC_VC_PRODUCTION = 295,
    IDC_VC_CAPITAL_SHIPS = 296,
    IDC_VC_HIGHEST_SCORE = 297,
};
typedef uint16_t NewGame3Control;

// VcrControl names the controls of the battle VCR dialog.
enum VcrControl {
    IDC_VCR_REW_ALL = 161,
    IDC_VCR_REW = 162,
    IDC_VCR_PLAY_PAUSE = 163,
    IDC_VCR_FWD = 164,
    IDC_VCR_FWD_ALL = 165,
};
typedef uint16_t VcrControl;

// HostModeControl names the controls of the host mode dialog.
enum HostModeControl {
    IDC_HOST_GENERATE_NOW = 1031,
    IDC_HOST_AUTO_GENERATE = 1032,
    IDC_HOST_GAME_NAME_TEXT = 1033,
    IDC_HOST_FILE_TEXT = 1034,
    IDC_HOST_PASSWORD = 2015,
    IDC_HOST_NEXT_YEAR_TEXT = 2016,
    IDC_HOST_TIME_SINCE_TEXT = 2017,
};
typedef uint16_t HostModeControl;

// HostOptionsControl names the controls of the auto generate options dialog; the second and third force generate options have no caption in the template.
enum HostOptionsControl {
    IDC_AUTOGEN_WHEN_ALL_IN = 1027,
    IDC_FORCE_GEN_NEVER = 2066,
    IDC_FORCE_GEN_OPTION_2 = 2067,
    IDC_FORCE_GEN_OPTION_3 = 2068,
};
typedef uint16_t HostOptionsControl;

// SlotControl names the controls of the ship designer dialog.
enum SlotControl {
    IDC_DESIGNER_COMPONENT_LIST = 2060,
    IDC_DESIGNER_SHIPS = 2064,
    IDC_DESIGNER_STARBASES = 2065,
    IDC_DESIGNER_EXISTING = 2066,
    IDC_DESIGNER_HULLS = 2067,
    IDC_DESIGNER_ENEMY_HULLS = 2068,
    IDC_DESIGNER_COMPONENTS = 2069,
};
typedef uint16_t SlotControl;

// RaceWizard1Control names the controls of the race wizard name and race page.
enum RaceWizard1Control {
    IDC_RACE_NAME = 268,
    IDC_RACE_PASSWORD = 269,
    IDC_RACE_HUMANOID = 271,
    IDC_RACE_RABBITOID = 272,
    IDC_RACE_INSECTOID = 273,
    IDC_RACE_NUCLEOTID = 274,
    IDC_RACE_SILICANOID = 275,
    IDC_RACE_ANTETHERAL = 276,
    IDC_RACE_RANDOM = 277,
    IDC_RACE_CUSTOM = 278,
    IDC_RACE_PLURAL_NAME = 2075,
};
typedef uint16_t RaceWizard1Control;

// RaceWizard2Control names the controls of the race wizard habitability page.
enum RaceWizard2Control {
    IDC_IMMUNE_TO_GRAVITY = 291,
    IDC_IMMUNE_TO_TEMPERATURE = 292,
    IDC_IMMUNE_TO_RADIATION = 293,
};
typedef uint16_t RaceWizard2Control;

// RaceWizard3Control names the controls of the race wizard economy page.
enum RaceWizard3Control {
    IDC_RACE_FACTORY_GERMANIUM_DISCOUNT = 291,
};
typedef uint16_t RaceWizard3Control;

// RaceWizard4Control names the controls of the race wizard primary racial trait page.
enum RaceWizard4Control {
    IDC_RACE_HYPER_EXPANSION = 271,
    IDC_RACE_SUPER_STEALTH = 272,
    IDC_RACE_WAR_MONGER = 273,
    IDC_RACE_CLAIM_ADJUSTER = 274,
    IDC_RACE_INNER_STRENGTH = 275,
    IDC_RACE_SPACE_DEMOLITION = 276,
    IDC_RACE_PACKET_PHYSICS = 277,
    IDC_RACE_INTERSTELLAR_TRAVELER = 278,
    IDC_RACE_ALTERNATE_REALITY = 279,
    IDC_RACE_JACK_OF_ALL_TRADES = 280,
};
typedef uint16_t RaceWizard4Control;

// RaceWizard5Control names the controls of the race wizard lesser racial trait page, in RaceGrbit order.
enum RaceWizard5Control {
    IDC_RACE_IMPROVED_FUEL_EFFICIENCY = 291,
    IDC_RACE_TOTAL_TERRAFORMING = 292,
    IDC_RACE_ADVANCED_REMOTE_MINING = 293,
    IDC_RACE_IMPROVED_STARBASES = 294,
    IDC_RACE_GENERALIZED_RESEARCH = 295,
    IDC_RACE_ULTIMATE_RECYCLING = 296,
    IDC_RACE_MINERAL_ALCHEMY = 297,
    IDC_RACE_NO_RAM_SCOOP_ENGINES = 298,
    IDC_RACE_CHEAP_ENGINES = 299,
    IDC_RACE_ONLY_BASIC_REMOTE_MINING = 300,
    IDC_RACE_NO_ADVANCED_SCANNERS = 301,
    IDC_RACE_LOW_STARTING_POPULATION = 302,
    IDC_RACE_BLEEDING_EDGE_TECHNOLOGY = 303,
    IDC_RACE_REGENERATING_SHIELDS = 304,
};
typedef uint16_t RaceWizard5Control;

// RaceWizard6Control names the controls of the race wizard research cost page.
enum RaceWizard6Control {
    IDC_RACE_ENERGY_COST_EXTRA = 271,
    IDC_RACE_ENERGY_COST_STANDARD = 272,
    IDC_RACE_ENERGY_COST_LESS = 273,
    IDC_RACE_WEAPONS_COST_EXTRA = 274,
    IDC_RACE_WEAPONS_COST_STANDARD = 275,
    IDC_RACE_WEAPONS_COST_LESS = 276,
    IDC_RACE_PROPULSION_COST_EXTRA = 277,
    IDC_RACE_PROPULSION_COST_STANDARD = 278,
    IDC_RACE_PROPULSION_COST_LESS = 279,
    IDC_RACE_CONSTRUCTION_COST_EXTRA = 280,
    IDC_RACE_CONSTRUCTION_COST_STANDARD = 281,
    IDC_RACE_CONSTRUCTION_COST_LESS = 282,
    IDC_RACE_ELECTRONICS_COST_EXTRA = 283,
    IDC_RACE_ELECTRONICS_COST_STANDARD = 284,
    IDC_RACE_ELECTRONICS_COST_LESS = 285,
    IDC_RACE_BIOTECH_COST_EXTRA = 286,
    IDC_RACE_BIOTECH_COST_STANDARD = 287,
    IDC_RACE_BIOTECH_COST_LESS = 288,
    IDC_RACE_START_HIGHER_TECH = 291,
};
typedef uint16_t RaceWizard6Control;

// ProductionControl names the controls of the production queue dialog.
enum ProductionControl {
    IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY = 139,
    IDC_PRODUCTION_AVAILABLE_ITEMS = 1046,
    IDC_PRODUCTION_QUEUE = 1047,
    IDC_PRODUCTION_ADD = 1048,
    IDC_PRODUCTION_REMOVE = 1049,
    IDC_PRODUCTION_CLEAR = 1069,
    IDC_PRODUCTION_ITEM_UP = 1081,
    IDC_PRODUCTION_ITEM_DOWN = 1082,
};
typedef uint16_t ProductionControl;

// MergeFleetsControl names the controls of the merge fleets dialog.
enum MergeFleetsControl {
    IDC_MERGE_FLEETS_LIST = 81,
    IDC_MERGE_FLEETS_SELECT_ALL = 2040,
    IDC_MERGE_FLEETS_UNSELECT_ALL = 2041,
};
typedef uint16_t MergeFleetsControl;

// BattlePlansControl names the controls of the battle plans dialog.
enum BattlePlansControl {
    IDC_BATTLE_PLAN_COPY = 1052,
    IDC_BATTLE_PLAN_DUMP_CARGO = 1053,
    IDC_BATTLE_PLAN_SELECT = 1054,
    IDC_BATTLE_PLAN_PRIMARY_TARGET = 1055,
    IDC_BATTLE_PLAN_SECONDARY_TARGET = 1056,
    IDC_BATTLE_PLAN_TACTIC = 1057,
    IDC_BATTLE_PLAN_ATTACK_WHO = 1058,
};
typedef uint16_t BattlePlansControl;

// RelationsControl names the controls of the player relations dialog.
enum RelationsControl {
    IDC_RELATIONS_PLAYER_LIST = 2003,
    IDC_RELATIONS_NEUTRAL = 2004,
    IDC_RELATIONS_FRIEND = 2005,
    IDC_RELATIONS_ENEMY = 2006,
};
typedef uint16_t RelationsControl;

// ResearchControl names the controls of the research dialog.
enum ResearchControl {
    IDC_RESEARCH_ENERGY = 1073,
    IDC_RESEARCH_WEAPONS = 1074,
    IDC_RESEARCH_PROPULSION = 1075,
    IDC_RESEARCH_CONSTRUCTION = 1076,
    IDC_RESEARCH_ELECTRONICS = 1077,
    IDC_RESEARCH_BIOTECH = 1078,
    IDC_RESEARCH_NEXT_FIELD = 1083,
};
typedef uint16_t ResearchControl;

// ZipProdControl names the controls of the production template dialog.
enum ZipProdControl {
    IDC_ZIP_PROD_QUEUE = 1047,
    IDC_ZIP_PROD_PRESET_1 = 1073,
    IDC_ZIP_PROD_PRESET_2 = 1074,
    IDC_ZIP_PROD_PRESET_3 = 1075,
    IDC_ZIP_PROD_PRESET_4 = 1076,
};
typedef uint16_t ZipProdControl;

// GaugeControl names the controls of the progress gauge dialog.
enum GaugeControl {
    IDC_GAUGE_TEXT = 1071,
};
typedef uint16_t GaugeControl;

// BrowserControl names the controls of the technology browser dialog.
enum BrowserControl {
    IDC_BROWSER_AVAILABLE_ONLY = 266,
    IDC_BROWSER_COMPONENT_CATEGORY = 267,
};
typedef uint16_t BrowserControl;

// TutorControl names the controls of the tutorial dialog.
enum TutorControl {
    IDC_TUTOR_HINT = 118,
    IDC_TUTOR_PANIC = 2503,
};
typedef uint16_t TutorControl;

// PanicControl names the controls of the tutorial panic dialog.
enum PanicControl {
    IDC_PANIC_REDO_TURN = 2505,
    IDC_PANIC_COMPLETE_TURN = 2506,
};
typedef uint16_t PanicControl;

// NewPasswordControl names the controls of the new password dialog.
enum NewPasswordControl {
    IDC_PASSWORD_CONFIRM = 269,
};
typedef uint16_t NewPasswordControl;

// ScoreControl names the controls of the score dialog.
enum ScoreControl {
    IDC_SCORE_SWITCH = 198,
};
typedef uint16_t ScoreControl;

enum ControlId {
    // Controls shared by several dialogs with the same meaning; a dialog's
    // own controls are in its dialog_controls enum.
    IDOK = 1,
    IDCANCEL = 2,
    IDHELP = 9,
    IDC_HELP = 118,

    // wizard and list navigation
    IDC_BACK = 1070,
    IDC_NEXT = 1071,
    IDC_FINISH = 1072,

    // name and text edits
    IDC_EDIT1 = 268,
    IDC_EDITTEXT = 268,
    IDC_EDITNAME = 2075,
    IDC_PASSWORD_STATUS_TEXT = 2018,
    IDC_COMBOBOX = 2074,

    // list item buttons
    IDC_RENAME = 1051,
    IDC_IMPORT = 2070,
    IDC_DELETE = 2071,
    IDC_EDIT = 2072,
    IDC_SHIPLIST = 1035,

    // save turn
    IDC_SAVE = 1065,
    IDC_SAVESUBMIT = 1066,
    IDC_NO_DON_T_SAVE = 1067,

    // ---- Menu and accelerator commands (WM_COMMAND ids) -------------------
    IDM_DEBUG_DUMP_FLEETS = 0x0053,   // DumpFleets()
    IDM_DEBUG_DUMP_PLANETS = 0x0054,  // DumpPlanets()
    IDM_DEBUG_DUMP_UNIVERSE = 0x0055, // DumpUniverse()

    // ---- About / score dialogs -------------------------------------------
    IDM_GAME_SCORE = 0x005F,  // Score dialog (one entry point)
    IDM_GAME_SCORE2 = 0x0060, // Score dialog (alternate entry point)
    IDM_HELP_ABOUT = 0x0063,  // About dialog

    // ---- Fleet waypoint editing ------------------------------------------
    IDM_FLEET_DELETE_WAYPOINT = 0x0067, // Delete current waypoint (confirm)
    IDM_FLEET_INSERT_WAYPOINT = 0x0068, // Waypoint insert/delete sibling command

    // ---- File / game lifecycle -------------------------------------------
    IDM_FILE_HOST_GAME = 0x0069,       // Host game ?
    IDM_TURN_WAIT_NEW = 0x006A,        // Turn > Wait for New
    IDM_FILE_OPEN_GAME = 0x006D,       // Open game
    IDM_FILE_NEW_GAME = 0x006E,        // New game wizard
    IDM_FILE_RETURN_TO_TITLE = 0x0071, // Close game, return to title screen

    // Toolbar/accelerator aliases that jump to the same paths
    IDM_TOOL_NEW_GAME = 0x0ED8,  // Alias: New game
    IDM_TOOL_OPEN_GAME = 0x0ED9, // Alias: Open game

    // ---- Commands (ship design / research / diplomacy) --------------------
    IDM_GAME_SHIP_BUILDER = 0x007D, // ShipBuilder
    IDM_GAME_RESEARCH = 0x007E,     // Research dialog

    // Diplomacy / battle plans / turn control cluster
    IDM_GAME_RELATIONS = 0x07D9,     // Relations dialog
    IDM_GAME_WAIT_FOR_TURN = 0x07DA, // Wait-for-turn dialog/command
    IDM_GAME_BATTLE_PLANS1 = 0x07DB, // Battle plans dialog
    IDM_GAME_BATTLE_PLANS2 = 0x07DC, // Battle plans dialog (alias)
    IDM_GAME_RELATIONS2 = 0x07DE,    // Relations dialog (alias)

    // ---- View / window layout --------------------------------------------
    IDM_VIEW_LAYOUT_0 = 0x0082, // Window layout 0
    IDM_VIEW_LAYOUT_1 = 0x0083, // Window layout 1
    IDM_VIEW_LAYOUT_2 = 0x0084, // Window layout 2 ("small" layout)

    // Browser toggle (menu vs alias ID)
    IDM_VIEW_BROWSER_TOGGLE = 0x0088,  // Toggle tech browser window
    IDM_VIEW_BROWSER_TOGGLE2 = 0x0100, // Alias: browser toggle

    // Help index (menu vs alias ID)
    IDM_HELP_CONTENTS = 0x008A,  // Help index/contents
    IDM_HELP_CONTENTS2 = 0x0101, // Alias: help index/contents

    // ---- Race wizards -----------------------------------------------------
    IDM_RACE_CREATE = 0x0081, // Race creation wizard (default players)
    IDM_RACE_EDIT1 = 0x009C,  // Race edit wizard (existing player)
    IDM_RACE_EDIT2 = 0x009D,  // Race edit wizard (alias)

    // ---- Reports ----------------------------------------------------------
    IDM_REPORT_PLANET = 0x08FD,      // Planet report
    IDM_REPORT_CYCLE = 0x08FE,       // Cycle report type
    IDM_REPORT_FLEET = 0x08FF,       // Fleet report
    IDM_REPORT_ENEMY_FLEET = 0x0900, // Enemy fleets report
    IDM_REPORT_BATTLE = 0x0901,      // Battles report

    // ---- MRU (Most Recently Used) slots ----------------------------------
    IDM_FILE_MRU1 = 0x10CC, // MRU slot 1
    IDM_FILE_MRU2 = 0x10CD, // MRU slot 2
    IDM_FILE_MRU3 = 0x10CE, // MRU slot 3
    IDM_FILE_MRU4 = 0x10CF, // MRU slot 4
    IDM_FILE_MRU5 = 0x10D0, // MRU slot 5
    IDM_FILE_MRU6 = 0x10D1, // MRU slot 6
    IDM_FILE_MRU7 = 0x10D2, // MRU slot 7
    IDM_FILE_MRU8 = 0x10D3, // MRU slot 8
    IDM_FILE_MRU9 = 0x10D4, // MRU slot 9

    // ---- Scanner zoom factors (radio group) -------------------------------
    IDM_SCAN_ZOOM_0 = 0x0F3D, // scanner zoom (entry 0)
    IDM_SCAN_ZOOM_1 = 0x0F3E, // scanner zoom (entry 1)
    IDM_SCAN_ZOOM_2 = 0x0F3F, // scanner zoom (entry 2)
    IDM_SCAN_ZOOM_3 = 0x0F40, // scanner zoom (entry 3)
    IDM_SCAN_ZOOM_4 = 0x0F41, // scanner zoom (entry 4) (baseline in code)
    IDM_SCAN_ZOOM_5 = 0x0F42, // scanner zoom (entry 5)
    IDM_SCAN_ZOOM_6 = 0x0F43, // scanner zoom (entry 6)
    IDM_SCAN_ZOOM_7 = 0x0F44, // scanner zoom (entry 7)
    IDM_SCAN_ZOOM_8 = 0x0F45, // scanner zoom (entry 8)

    // ---- Turn ending / host/generate variants ------------------------------
    IDM_TURN_END_A = 0x0EDA, // end turn variant A
    IDM_TURN_END_B = 0x0EDB, // end turn variant B (toggles an internal bit)

    // ---- Dynamic popup range ----------------------------------------------
    IDM_POPUP_BASE = 15000, // Dynamic popup items start here (inferred)

    // ---- Debug: force-generate turns (decompiler had type confusion) -------
    IDM_DEBUG_GEN_10_TURNS = 21000,  // generate 10 turns (inferred)
    IDM_DEBUG_GEN_100_TURNS = 21001, // generate 100 turns (0x5209)
    IDM_DEBUG_GEN_1000_TURNS = 21002,
    IDM_VIEW_PLAYER_COLORS = 0x098D,
    IDM_HELP_CONTEXT_13002 = 0x09C1,
    IDM_HELP_INTRO = 0x09C2,
    IDM_HELP_TUTORIAL2 = 0x09C4,
    IDM_HELP_TUTORIAL = 0x09C5,
    IDM_FILE_EXIT = 0x0EE2,
    IDM_FRAME_POST_OPEN = 0x0FA1,
    IDM_VIEW_FIND = 0x1068,
    IDM_VIEW_FIND2 = 0x1069,

    IDM_GAME_RESEARCH2 = 0x0087,
    IDM_GAME_SHIP_BUILDER2 = 0x0089,
    IDM_VIEW_GAME_PARAMS = 0x009E,
    IDM_VIEW_GAME_PARAMS2 = 0x009F,
    IDM_VIEW_TOOLBAR = 0x00B3,
    IDM_FILE_PRINT_MAP = 0x00D5,
    IDM_TITLE_NEW_GAME = 0x00FA,
    IDM_TITLE_OPEN_GAME = 0x00FB,
    IDM_TITLE_CONTINUE = 0x00FC,
    IDM_TITLE_EXIT = 0x00FD,
    IDM_CMD_CHANGE_PASSWORD = 0x010E,
    IDM_TURN_SAVE_SUBMIT = 0x0428,

    IDM_TURN_GENERATE = 0x006C,
    IDM_FILE_SAVE = 0x006F,

};
typedef uint16_t ControlId;

/* Numeric cursor resources, named after the hcur globals they load into;
 * the others are named (SCANNERCUR, ...). */
#undef IDC_HAND
enum CursorId {
    IDC_NO_WAY = 121,
    IDC_TRASH_CAN = 122,
    IDC_RESIZE_WE = 258,
    IDC_RESIZE_NS = 260,
    IDC_RESIZE_4WAY = 263,
    IDC_ARROW_HELP = 264,
    IDC_HAND = 265,
};
typedef uint16_t CursorId;

/* Numeric bitmap resources; the others are named (CARGOBMP, ...). IDB_ ones
 * are loaded with LoadBitmap, IDDIB_ ones as DIBs through FindResource. */
enum BitmapId {
    IDDIB_PLAYER_ICONS_TINY = 79,
    IDDIB_PLAYER_ICONS_SMALL = 80,
    IDDIB_THING_ICONS = 87,
    IDDIB_SCANNER_TOOLBAR = 88,
    IDDIB_PLANET_ICONS = 112,
    IDB_EMPTY_HULL_SLOT = 119,
    IDDIB_PLAYER_ICONS = 133,
    IDB_MSGFILTER_CHECKBOX = 134,
    IDB_TOOLBAR = 178,
    IDB_FILTER_CHECKBOX_MONO = 199,
    IDB_FONT_DIGITS = 249,
    IDDIB_SPLASH = 449,
    IDB_MINESPAT_1 = 460,
    IDB_MINESPAT_2 = 461,
    IDB_MINESPAT_3 = 462,
    IDDIB_TECH_ICONS_1 = 500,
    IDDIB_TECH_ICONS_2 = 501,
    IDDIB_TECH_ICONS_3 = 502,
    IDDIB_TECH_ICONS_4 = 503,
    IDDIB_TECH_ICONS_5 = 504,
    IDDIB_TECH_ICONS_6 = 505,
    IDDIB_TECH_ICONS_7 = 506,
    IDDIB_HULL_ICONS_1 = 552,
    IDDIB_HULL_ICONS_2 = 553,
    IDDIB_HULL_ICONS_3 = 554,
    IDDIB_HULL_ICONS_4 = 555,
    IDDIB_HULL_ICONS_5 = 556,
    IDDIB_HULL_ICONS_SMALL_1 = 557,
    IDDIB_HULL_ICONS_SMALL_2 = 558,
    IDDIB_HULL_ICONS_SMALL_3 = 559,
    IDDIB_HULL_ICONS_SMALL_4 = 560,
    IDDIB_HULL_ICONS_SMALL_5 = 561,
    IDDIB_NUM_DESIGNS_PLATE = 1079,
};
typedef uint16_t BitmapId;

enum AcceleratorId {
    IDA_MAIN = 116,
    IDA_TITLE = 1080,
};
typedef uint16_t AcceleratorId;

/* Tutorial game files, each stored as its own custom resource type. */
enum TutorialResourceId {
    RT_TUTORIAL_HST = 10000,
    IDR_TUTORIAL_HST = 10001,
    RT_TUTORIAL_M1 = 10002,
    IDR_TUTORIAL_M1 = 10003,
    RT_TUTORIAL_M2 = 10004,
    IDR_TUTORIAL_M2 = 10005,
};
typedef uint16_t TutorialResourceId;

enum VictoryCondition {
    vcOwnsPercentPlanets = 0,     /* "Owns % of all planets." */
    vcAttainsTechLevel = 1,       /* "Attains Tech X in Y fields." (level) */
    vcAttainsTechFields = 2,      /* number of tech fields */
    vcExceedsScore = 3,           /* "Exceeds a score of X." */
    vcExceedsSecondPlaceBy = 4,   /* "Exceeds second place score by X." */
    vcProductionCapacity = 5,     /* "Has a production capacity of X thousand." */
    vcOwnsCapitalShips = 6,       /* "Owns X capital ships." */
    vcHighestScoreAfterYears = 7, /* "Has the highest score after X years." */
    vcMeetsNumCriteria = 8,       /* "Winner must meet X of the above selected criteria." */
    vcMinYearsBeforeWin = 9       /* "At least X years must pass before a winner is declared." */
};
typedef uint16_t VictoryCondition;

enum MdOpenFlags {
    /* access + share combinations */
    mdRead = 0x0020,      /* OF_READ | OF_SHARE_DENY_WRITE */
    mdReadWrite = 0x0012, /* OF_READWRITE | OF_SHARE_EXCLUSIVE */

    /* create/truncate */
    mdCreate = 0x1012, /* OF_CREATE | OF_READWRITE | OF_SHARE_EXCLUSIVE */

    /* Stars!-specific modifier */
    mdNoOpenErr = 0x4000,
};
typedef uint16_t MdOpenFlags;

enum TaskType {
    grTaskNone = 0,
    grTaskXfer = 1, /* transport / transfer cargo */
    grTaskColonize = 2,
    grTaskMine = 3, /* remote mining */
    grTaskMerge = 4,
    grTaskScrap = 5,
    grTaskLayMines = 6,
    grTaskPatrol = 7,
    grTaskAutoRoute = 8, /* auto-route / auto-order */
    grTaskGive = 9,
    grTaskAny = 0xffff,
};
typedef uint16_t TaskType;

enum XferActionType {
    iActionNone = 0, /* implicit / cleared */

    iActionLoadAll = 1,     /* "Load All Available"        */
    iActionUnloadAll = 2,   /* "Unload All"                */
    iActionLoadExact = 3,   /* "Load Exactly..."           */
    iActionUnloadExact = 4, /* "Unload Exactly..."         */
    iActionFillPercent = 5, /* "Fill Up to %..."           */
    iActionWaitPercent = 6, /* "Wait for %..."             */
    iActionLoadDunnage = 7, /* "Load Dunnage"              */
    iActionSetAmount = 8,   /* "Set Amount to..."          */
    iActionSetWaypoint = 9, /* "Set Waypoint to..."        */
    /* iActionLoadOptimal is encoded via iActionLoadDunnage + fuel path */
};
typedef uint16_t XferActionType;

enum MdTarget {
    mdTargetNone = 0,              /* "None/Disengage" */
    mdTargetAny = 1,               /* "Any" */
    mdTargetStarbase = 2,          /* "Starbase" */
    mdTargetArmedShips = 3,        /* "Armed Ships" */
    mdTargetBombersFreighters = 4, /* "Bombers/Freighters" */
    mdTargetUnarmedShips = 5,      /* "Unarmed Ships" */
    mdTargetFuelTransports = 6,    /* "Fuel Transports" */
    mdTargetFreighters = 7,        /* "Freighters" */
};
typedef uint16_t MdTarget;

enum BattleTactic {
    mdTacticDisengage = 0,             /* "Disengage" */
    mdTacticDisengageIfChallenged = 1, /* "Disengage if challenged" */
    mdTacticMinDamageToSelf = 2,       /* "Minimize damage to self" */
    mdTacticMaxNetDamage = 3,          /* "Maximize net damage" */
    mdTacticMaxDamageRatio = 4,        /* "Maximize damage ratio" */
    mdTacticMaxDamage = 5,             /* "Maximize damage" */
};
typedef uint16_t BattleTactic;

enum GrfWeapon {
    bitFBeamLow = 0x0001,
    bitFBeamHigh = 0x0002,
    bitFTorp = 0x0004,
    bitFMissile = 0x0008,
    bitFNoHit = 0x0040, /* torpedo damage fully absorbed: the VCR draws no hit */
    bitFDeflected = 0x0080,
};
typedef uint16_t GrfWeapon;

// AI part IDs select ordered component preference groups in vrgAiParts;
// generated from data/ai/parts.yaml (dataids.h).
typedef uint16_t AiPartPreference;

// A recipe in vrgTDAip, valued by its byte offset there; vrgTDIshAip lists
// them in order. Generated from data/ai/ (dataids.h).
typedef uint16_t TurinDroneRecipe;

// A recipe in vrgRobAip, valued by its byte offset there; vrgRobIshAip lists
// them in order. Generated from data/ai/ (dataids.h).
typedef uint16_t RobotoidRecipe;

// A recipe in vrgISAip, valued by its byte offset there; vrgISIshAip lists
// them in order. Generated from data/ai/ (dataids.h).
typedef uint16_t ISRecipe;

// A recipe in vrgMacAip, valued by its byte offset there; vrgMacIshAip lists
// them in order. Generated from data/ai/ (dataids.h).
typedef uint16_t MacintiRecipe;

// A recipe in vrgCyberAip, valued by its byte offset there; vrgCyberIshAip lists
// them in order. Generated from data/ai/ (dataids.h).
typedef uint16_t CybertronRecipe;

// A starbase recipe in vrgSBAip, valued by its byte offset there; generated
// from data/ai/starbases.yaml (dataids.h). aiSbRecipeAuto asks
// FCreateAiStarbase to choose.
enum AiStarbaseRecipeSentinel {
    aiSbRecipeAuto = -1,
};
typedef int16_t AiStarbaseRecipe;

// Squared packet attack distance thresholds for one or two mass drivers.
enum AiPacketDistanceSquared {
    aiPacketDistanceSquaredSingleMassDriver = 7056,
    aiPacketDistanceSquaredTwoMassDrivers = 50625,
};
typedef uint16_t AiPacketDistanceSquared;

// AiRace identifies a computer player's race and the AI routine that plays it.
enum AiRace {
    idAiRobotoid = 0,
    idAiTurinDrone = 1,
    idAiAutomitron = 2,
    idAiRototill = 3,
    idAiCybertron = 4,
    idAiMacinti = 5,
    idAiRandom = 6, // replaced with a random race when the game is created
    idAiMaid = 7,   // housekeeping AI that runs a human player's empire
};
typedef uint16_t AiRace;

// MineFieldType is a minefield's kind.
enum MineFieldType {
    mineStandard = 0,
    mineHeavy = 1,
    mineSpeedBump = 2,
    mineAll = 0xffff,
    mineNone = 0xffff,
};
typedef uint16_t MineFieldType;

// ScanView is the scanner's view mode, the low nibble of grbitScan.
enum ScanView {
    scanViewNormal = 0,
    scanViewSurfaceMinerals = 1,
    scanViewMineralConc = 2,
    scanViewPlanetValue = 3,
    scanViewPopulation = 4,
    scanViewNoPlayerInfo = 5,
};
typedef uint16_t ScanView;

// GrbitScan holds the scanner's overlay and filter toggles above the view mode
// in grbitScan.
enum GrbitScan {
    grbitScanViewMask = 0x000f, // ScanView
    grbitScanAddWaypoints = 0x0010,
    grbitScanCoverage = 0x0020,
    grbitScanMineFields = 0x0040,
    grbitScanFleetPaths = 0x0080,
    grbitScanIdleFleets = 0x0100,
    grbitScanDesignFilter = 0x0200,
    grbitScanPlanetNames = 0x0400,
    grbitScanEnemyFilter = 0x0800,
    grbitScanShipCounts = 0x1000,
    grbitScanPlayerColors = 0x2000,
    grbitScanToggleMask = 0x3ff0,
};
typedef uint16_t GrbitScan;

// ToolbarButton is a scanner toolbar button; the negative values are layout
// entries of vrgTBBtn.
enum ToolbarButton {
    tbScannerRange = -3, // scanner range readout
    tbSpacer = -2,
    tbSeparator = -1,
    tbNormalView = 0,
    tbSurfaceMineralView = 1,
    tbMineralConcView = 2,
    tbPlanetValueView = 3,
    tbPopulationView = 4,
    tbNoPlayerInfoView = 5,
    tbAddWaypoints = 6,
    tbScannerCoverage = 7,
    tbMineFields = 8,
    tbFleetPaths = 9,
    tbIdleFleets = 10,
    tbPlanetNames = 11,
    tbShipDesignFilter = 12,
    tbShipDesignFilterMenu = 13,
    tbEnemyClassFilter = 14,
    tbEnemyClassFilterMenu = 15,
    tbZoomMenu = 16,
    tbShipCounts = 17,
};
typedef int16_t ToolbarButton;

// UniverseSize is the galaxy size; its width is 400 * (size + 1) light years.
enum UniverseSize {
    sizeTiny = 0,
    sizeSmall = 1,
    sizeMedium = 2,
    sizeLarge = 3,
    sizeHuge = 4,
};
typedef uint16_t UniverseSize;

// UniverseDensity is the galaxy's planet density.
enum UniverseDensity {
    densitySparse = 0,
    densityNormal = 1,
    densityDense = 2,
    densityPacked = 3,
};
typedef uint16_t UniverseDensity;

// StartDistance is the distance between players' homeworlds.
enum StartDistance {
    startDistClose = 0,
    startDistModerate = 1,
    startDistFarther = 2,
    startDistDistant = 3,
};
typedef uint16_t StartDistance;

// MsgGoto is a message's goto target: a planet id (>= 0), a fleet id, a
// battle id | 0x4000, a part as 0xc000 | (hst bit << 8) | iItem, or one of
// these.
enum MsgGoto {
    gotoBattleReport = -7,
    gotoThing = -6, // the thing id is the message's first parameter
    gotoScore = -4,
    gotoShipDesign = -3,
    gotoResearch = -2,
    gotoNone = -1,
    gotoBattle = 0x4000,
    gotoRelations = 0x4800,
};
typedef int16_t MsgGoto;

// MdMsgObj is what the message window's goto button opens.
enum MdMsgObj {
    mdMsgObjNone = 0,
    mdMsgObjPlanet = 1,
    mdMsgObjFleet = 2,
    mdMsgObjResearch = 3,
    mdMsgObjPart = 4,
    mdMsgObjShipDesign = 5,
    mdMsgObjBattle = 6,
    mdMsgObjRelations = 7,
    mdMsgObjScore = 8,
    mdMsgObjThing = 10,
    mdMsgObjBattleReport = 11,
};
typedef uint16_t MdMsgObj;

// TileBits selects the planet or fleet detail tiles DrawPlanShip draws. A bit
// shared by both tile sets is named for both.
enum TileBits {
    tileMineralsOrCargo = 0x0001,
    tileShipList = 0x0004,
    tilePlanetStats = 0x0008,
    tileFleetOrders = 0x0020,
    tileProductionOrOrbit = 0x0040,
    tileBitmap = 0x0080,
    tileStarbaseOrWaypoint = 0x0100,
    tileFleetComp = 0x0200,
    tileAll = 0x0fff,
    tileMinimized = 0x4000,
    tileErase = 0x8000,
};
typedef uint16_t TileBits;

// ReportType is a report dialog's report.
enum ReportType {
    rptPlanets = 0,
    rptFleets = 1,
    rptEnemyFleets = 2,
    rptBattles = 3,
};
typedef uint16_t ReportType;

// PlanetReportColumn is a column of the planet report.
enum PlanetReportColumn {
    colPlanetName = 0,
    colPlanetStarbase = 1,
    colPlanetPopulation = 2,
    colPlanetCapacity = 3,
    colPlanetValue = 4,
    colPlanetProduction = 5,
    colPlanetMines = 6,
    colPlanetFactories = 7,
    colPlanetDefense = 8,
    colPlanetMinerals = 9,
    colPlanetMiningRate = 10,
    colPlanetMinConc = 11,
    colPlanetResources = 12,
    colPlanetDriverDest = 13,
    colPlanetRoutingDest = 14,
    colPlanetCount = 15,
};
typedef uint16_t PlanetReportColumn;

// FleetReportColumn is a column of the fleet report.
enum FleetReportColumn {
    colFleetName = 0,
    colFleetId = 1,
    colFleetLocation = 2,
    colFleetDestination = 3,
    colFleetEta = 4,
    colFleetTask = 5,
    colFleetFuel = 6,
    colFleetCargo = 7,
    colFleetComposition = 8,
    colFleetCloak = 9,
    colFleetBattlePlan = 10,
    colFleetMass = 11,
    colFleetCount = 12,
};
typedef uint16_t FleetReportColumn;

// EnemyFleetReportColumn is a column of the other players' fleet report.
enum EnemyFleetReportColumn {
    colEnemyFleetName = 0,
    colEnemyFleetId = 1,
    colEnemyFleetLocation = 2,
    colEnemyFleetWarp = 3,
    colEnemyFleetMass = 4,
    colEnemyFleetComposition = 5,
    colEnemyFleetShips = 6,
    colEnemyFleetUnarmed = 7,
    colEnemyFleetScout = 8,
    colEnemyFleetWarship = 9,
    colEnemyFleetBomber = 10,
    colEnemyFleetUtility = 11,
    colEnemyFleetCount = 12,
};
typedef uint16_t EnemyFleetReportColumn;

// BattleReportColumn is a column of the battle report.
enum BattleReportColumn {
    colBattleLocation = 0,
    colBattleStarbase = 1,
    colBattleSides = 2,
    colBattleUnits = 3,
    colBattleOurs = 4,
    colBattleTheirs = 5,
    colBattleUnarmed = 6,
    colBattleScout = 7,
    colBattleWarship = 8,
    colBattleBomber = 9,
    colBattleUtility = 10,
    colBattleOurDead = 11,
    colBattleTheirDead = 12,
    colBattleOursLeft = 13,
    colBattleTheirsLeft = 14,
    colBattleCount = 15,
};
typedef uint16_t BattleReportColumn;

// WindowLayout is the main window layout, chosen by screen size.
enum WindowLayout {
    layoutLarge = 0,
    layoutMedium = 1,
    layoutSmall = 2,
};
typedef uint16_t WindowLayout;

// ScanZoom is the scanner zoom level.
enum ScanZoom {
    zoom25 = -4,
    zoom38 = -3,
    zoom50 = -2,
    zoom75 = -1,
    zoom100 = 0,
    zoom125 = 1,
    zoom150 = 2,
    zoom200 = 3,
    zoom400 = 4,
};
typedef int16_t ScanZoom;

// MdMark is the turn-file flag FMarkFile sets and FCheckFile tests.
enum MdMark {
    mdMarkInUse = 1,
    mdMarkDone = 2,
    mdMarkMulti = 4,
    mdMarkAi = 8, // the player is run by the housekeeping AI
};
typedef uint16_t MdMark;

// HostTimer is the job of the host-mode timer.
enum HostTimer {
    hostTimerAutoGen = 13,   // poll for turns to generate
    hostTimerWaitTurn = 14,  // wait for the next turn after submitting
    hostTimerTurnReady = 15, // flash the window when a new turn is ready
};
typedef uint16_t HostTimer;

// ProgressStep is an UpdateProgressGauge argument that advances the gauge
// instead of setting it.
enum ProgressStep {
    progressStep1 = -927,
    progressStep4 = -926,
};
typedef int16_t ProgressStep;

// AddItemMode is where AddItemToQueue puts an item in the production queue.
enum AddItemMode {
    addItemFront = 0,
    addItemEnd = 1,
    addItemReplace = 2, // clear the queue first
};
typedef uint16_t AddItemMode;

// MainMenu is a top-level menu's position in the frame menu bar.
enum MainMenu {
    menuFile = 0,
    menuView = 1,
    menuTurn = 2,
    menuCommands = 3,
    menuReport = 4,
    menuHelp = 5,
};
typedef uint16_t MainMenu;

// TutorShipBuilderAction is the ship designer button FTutorialEnabledShipBuilder
// checks against the tutorial's current step.
enum TutorShipBuilderAction {
    tutsbDelete = 0,
    tutsbCopy = 1,
    tutsbEdit = 2,
    tutsbAccept = 3,     // OK on an edited design
    tutsbCancelEdit = 4, // Cancel on an edited design
};
typedef uint16_t TutorShipBuilderAction;

// HelpContextId is a topic of stars!.hlp, as WinHelp's HELP_CONTEXT data
// selects it: the [MAP] numbers of the help file's |CTXOMAP, named by topic
// title. Many share their number with the dialog control they explain.
// Generated by scripts/hlp-context-enum.py.
enum HelpContextId {
    idhStarsPlayersGuideContents = 1,                         // Stars! Player's Guide - Contents
    idhIntroductionAndPlayerSupport = 2,                      // Introduction and Player Support
    idhTheStarsScreen = 3,                                    // The Stars! Screen
    idhStarsDialogs = 4,                                      // Stars! dialogs
    idhPlayingStars = 6,                                      // Playing Stars!
    idhTheGuts = 7,                                           // The Guts
    idhHowTo = 8,                                             // How To...
    idhSetupAndHosting = 9,                                   // Setup and Hosting
    idhNewGameSetupBasic = 1002,                              // New Game Setup (Basic)
    idhBeginTutorial = 1003,                                  // Begin Tutorial
    idhDifficultyLevel = 1004,                                // Difficulty Level
    idhTinyUniverse = 1005,                                   // Tiny Universe
    idhSmallUniverse = 1006,                                  // Small Universe
    idhMediumUniverse = 1007,                                 // Medium Universe
    idhLargeUniverse = 1008,                                  // Large Universe
    idhHugeUniverse = 1009,                                   // Huge Universe
    idhPlayerRace = 1010,                                     // Player Race
    idhNewGameSetupAdvanced = 1011,                           // New Game Setup (Advanced)
    idhStep1SpecifyingTheUniverse = 1012,                     // Step 1: Specifying the Universe
    idhGameName = 1013,                                       // Game Name
    idhDensity = 1014,                                        // Density
    idhPlayerPositions = 1015,                                // Player Positions
    idhBeginnerUnlimitedMinerals = 1016,                      // Beginner: Unlimited Minerals
    idhSlowerTechAdvances = 1017,                             // Slower Tech Advances
    idhComputerPlayersFormAlliances = 1018,                   // Computer Players Form Alliances
    idhAcceleratedBBSPlay = 1019,                             // Accelerated BBS Play
    idhStep2SpecifyingThePlayers = 1020,                      // Step 2: Specifying the Players
    idhStep3VictoryConditions = 1021,                         // Step 3: Victory Conditions
    idhCustomRaceWizard = 1022,                               // Custom Race Wizard
    idhStep1BasicDefinition = 1023,                           // Step 1: Basic Definition
    idhRaceNameAndPassword = 1024,                            // Race Name and Password
    idhPredefinedRaces = 1025,                                // Predefined Races
    idhLeftoverAdvantagePointsSurfaceMinerals = 1026,         // Leftover Advantage Points -- Surface Minerals
    idhLeftoverAdvantagePointsMines = 1028,                   // Leftover Advantage Points -- Mines
    idhLeftoverAdvantagePointsFactories = 1029,               // Leftover Advantage Points -- Factories
    idhLeftoverAdvantagePointsDefenses = 1030,                // Leftover Advantage Points -- Defenses
    idhRaceIcon = 1031,                                       // Race Icon
    idhStep2PrimaryRacialTraits = 1032,                       // Step 2: Primary Racial Traits
    idhHyperExpansion = 1033,                                 // Hyper-Expansion
    idhSuperStealth = 1034,                                   // Super-Stealth
    idhWarMonger = 1035,                                      // War Monger
    idhInnerStrength = 1036,                                  // Inner-Strength
    idhSpaceDemolition = 1037,                                // Space Demolition
    idhPacketPhysics = 1038,                                  // Packet Physics
    idhInterstellarTraveller = 1039,                          // Interstellar Traveller
    idhJackOfAllTrades = 1040,                                // Jack of All Trades
    idhStep3LesserTraitsPlayerRace = 1041,                    // Step 3: Lesser Traits (Player Race)
    idhImprovedFuelEfficiency = 1042,                         // Improved Fuel Efficiency
    idhTotalTerraforming1043 = 1043,                          // Total Terraforming
    idhImprovedStarbases = 1044,                              // Improved Starbases
    idhGeneralizedResearch = 1045,                            // Generalized Research
    idhMineralAlchemy1046 = 1046,                             // Mineral Alchemy
    idhNoRamscoopEngines = 1047,                              // No Ramscoop Engines
    idhCheapEngines = 1048,                                   // Cheap Engines
    idhOnlyBasicRemoteMining = 1049,                          // Only Basic Remote Mining
    idhNoAdvancedScanners = 1050,                             // No Advanced Scanners
    idhLowStartingPopulation = 1051,                          // Low Starting Population
    idhRegeneratingShields = 1052,                            // Regenerating Shields
    idhStep4PopulationGrowthFactors = 1053,                   // Step 4: Population Growth Factors
    idhGrowthConditions = 1054,                               // Growth Conditions
    idhMaximumPopulationGrowth = 1055,                        // Maximum Population Growth
    idhStep5PopulationEfficiencyPlayerRace = 1056,            // Step 5: Population Efficiency (Player Race)
    idhStep6ResearchCostsPlayerRace = 1057,                   // Step 6: Research Costs (Player Race)
    idhFinishAndSave = 1058,                                  // Finish and Save
    idhProductionDialog = 1059,                               // Production Dialog
    idhProductionInventory = 1060,                            // Production Inventory
    idhProductionQueue = 1061,                                // Production Queue
    idhShipDesigner = 1066,                                   // Ship Designer
    idhShipSchematic = 1068,                                  // Ship Schematic
    idhShipComponentList = 1069,                              // Ship Component List
    idhResearchDialog = 1070,                                 // Research Dialog
    idhTechnologyStatus = 1071,                               // Technology Status
    idhExpectedResearchBenefits = 1072,                       // Expected Research Benefits
    idhCurrentlyResearching = 1073,                           // Currently Researching
    idhResourceAllocation = 1074,                             // Resource Allocation
    idhCargoTransferDialogs = 1075,                           // Cargo Transfer Dialogs
    idhBetweenYourPlanetAndYourFleet = 1076,                  // Between Your Planet And Your Fleet
    idhBetweenYourFleetAndAPlanet = 1077,                     // Between Your Fleet and a Planet
    idhBetweenYourFleets = 1078,                              // Between Your Fleets
    idhBetweenYourFleetAndAnOpponentsFleet = 1079,            // Between Your Fleet and an Opponents Fleet
    idhShipTransferDialog = 1080,                             // Ship Transfer Dialog
    idhBattlePlansDialog = 1081,                              // Battle Plans Dialog
    idhBattleVCR = 1082,                                      // Battle VCR
    idhPlayerRelationsDialog = 1083,                          // Player Relations Dialog
    idhChangePassword = 1084,                                 // Change Password
    idhFindPlanetOrFleet = 1085,                              // Find Planet or Fleet
    idhHostModeDialog = 1088,                                 // Host Mode Dialog
    idhRenameFleetDialog = 1095,                              // Rename Fleet dialog
    idhAdvancedRemoteMining = 1096,                           // Advanced Remote Mining
    idhPublicPlayerScores = 1097,                             // Public Player Scores
    idhCustomZipOrdersDialog = 1098,                          // Custom Zip Orders dialog
    idhLeftoverAdvantagePointsMineralConcentration = 1099,    // Leftover Advantage Points  Mineral Concentration
    idhClaimAdjuster = 1100,                                  // Claim Adjuster
    idhAlternateReality = 1101,                               // Alternate Reality
    idhUltimateRecycling = 1102,                              // Ultimate Recycling
    idhBleedingEdgeTechnology = 1103,                         // Bleeding Edge Technology
    idhNoRandomEvents = 1104,                                 // No Random Events
    idhCustomizeProductionTemplatesDialog = 1106,             // Customize Production Templates dialog
    idhMergeFleetsDialog = 1107,                              // Merge Fleets dialog
    idhGalaxyClumping = 1108,                                 // Galaxy Clumping
    idhScoreSheet = 1109,                                     // Score sheet
    idhChangingTheBasicLayout = 1502,                         // Changing the Basic Layout
    idhShrinkingAndGrowingPanes = 1503,                       // Shrinking and Growing Panes
    idhMovingAndCollapsingTiles = 1504,                       // Moving and Collapsing Tiles
    idhCommandingAPlanet = 1505,                              // Commanding a Planet
    idhPlanetTile = 1506,                                     // Planet Tile
    idhProductionTile = 1507,                                 // Production Tile
    idhStatusTile = 1508,                                     // Status Tile
    idhMineralsOnHandTile = 1509,                             // Minerals on Hand Tile
    idhFleetsInOrbitTile = 1510,                              // Fleets in Orbit Tile
    idhStarbaseTile = 1511,                                   // Starbase Tile
    idhCommandingAFleet = 1512,                               // Commanding a Fleet
    idhFleetTile = 1513,                                      // Fleet Tile
    idhLocationTile = 1514,                                   // Location Tile
    idhFuelAndCargoTile = 1515,                               // Fuel and Cargo Tile
    idhFleetCompositionTile = 1516,                           // Fleet Composition Tile
    idhOtherFleetsHereTile = 1517,                            // Other Fleets Here Tile
    idhFleetWaypointsTile = 1518,                             // Fleet Waypoints Tile
    idhWaypointTaskTile = 1519,                               // Waypoint Task Tile
    idhTransport = 1520,                                      // Transport
    idhColonize = 1522,                                       // Colonize
    idhPatrol = 1523,                                         // Patrol
    idhRemoteMining1524 = 1524,                               // Remote Mining
    idhScrapFleet = 1525,                                     // Scrap Fleet
    idhSelectingAnObjectToCommand = 1526,                     // Selecting an Object to Command
    idhObtainingAPlanetOrFleetSummary = 1527,                 // Obtaining a Planet or Fleet Summary
    idhLayMineFields = 1528,                                  // Lay Mine Fields
    idhMergeWithFleet = 1530,                                 // Merge with Fleet
    idhRoute = 1531,                                          // Route
    idhRemoteTerraforming1532 = 1532,                         // Remote Terraforming
    idhTransferFleet = 1533,                                  // Transfer Fleet
    idhTheGutsOfCombat = 2008,                                // The Guts of Combat
    idhAboutTheBattleBoard = 2009,                            // About the Battle Board
    idhDamageRepair = 2013,                                   // Damage Repair
    idhMovementInitiativeAndFiringInBattle = 2014,            // Movement, Initiative and Firing in Battle
    idhWeaponProperties = 2015,                               // Weapon Properties
    idhTheGutsOfMassDrivers = 2016,                           // The Guts of Mass Drivers
    idhFilesUsedInStars = 2017,                               // Files Used in Stars!
    idhArmorShieldsAndDamage = 2018,                          // Armor, Shields and Damage
    idhTheGutsOfMinefields = 2022,                            // The Guts of Minefields
    idhAlternateRealityRaces = 2024,                          // Alternate Reality Races
    idhTheGutsOfCloaking = 2025,                              // The Guts of Cloaking
    idhAboutDataTables = 2501,                                // About Data Tables
    idhArmor = 2502,                                          // Armor
    idhBeamWeapons = 2503,                                    // Beam Weapons
    idhBombsTable = 2504,                                     // Bombs table
    idhEnginesTable = 2506,                                   // Engines table
    idhScannersTable = 2508,                                  // Scanners table
    idhShieldsTable = 2509,                                   // Shields table
    idhMiningTable = 2511,                                    // Mining table
    idhTerraformingTable = 2513,                              // Terraforming table
    idhOrbitalDevicesTable = 2516,                            // Orbital Devices table
    idhElectricalDevicesTable = 2518,                         // Electrical Devices table
    idhMechanicalDevicesTable = 2519,                         // Mechanical Devices table
    idhMineLayingTable = 2520,                                // Mine Laying table
    idhPlanetaryInstallationsTable = 2521,                    // Planetary Installations table
    idhShipHullsTable = 2522,                                 // Ship Hulls table
    idhStarbaseHullsTable = 2523,                             // Starbase Hulls table
    idhTorpedoesTable = 2524,                                 // Torpedoes table
    idhPlanets3002 = 3002,                                    // Planets
    idhYourHomeWorldAndOtherInhabitedPlanets = 3003,          // Your Home World and Other Inhabited Planets
    idhPopulation = 3004,                                     // Population
    idhMinerals = 3005,                                       // Minerals
    idhMines = 3006,                                          // Mines
    idhFactories = 3007,                                      // Factories
    idhTerraforming = 3009,                                   // Terraforming
    idhTypesOfTerraformingTechnology = 3011,                  // Types of Terraforming Technology
    idhTotalTerraforming3012 = 3012,                          // Total Terraforming
    idhBuildingPlanetaryDefenses = 3013,                      // Building Planetary Defenses
    idhPlanetBasedScanners = 3014,                            // Planet-based Scanners
    idhOrbitalDevices = 3015,                                 // Orbital Devices
    idhStarbases = 3016,                                      // Starbases
    idhStargates = 3017,                                      // Stargates
    idhMassDriverBasics = 3018,                               // Mass Driver Basics
    idhProduction = 3019,                                     // Production
    idhHowProductionWorks = 3020,                             // How Production Works
    idhAddingAnItemToTheProductionQueue = 3021,               // Adding an Item to the Production Queue
    idhAddAnItemToTheTopOfTheQueue = 3022,                    // Add an item to the top of the queue
    idhAddAnItemToTheMiddleOfTheQueue = 3023,                 // Add an item to the middle of the queue
    idhAddAnItemToTheBottomOfTheQueue = 3024,                 // Add an item to the bottom of the queue
    idhMoveAnItemInTheQueue = 3025,                           // Move an item in the queue
    idhRemovingAnItemFromTheProductionQueue = 3026,           // Removing an Item from the Production Queue
    idhClearingTheProductionQueue = 3027,                     // Clearing the Production Queue
    idhUnblockingAProductionQueue = 3028,                     // Unblocking a Production Queue
    idhAddingAutoBuildItemsToTheQueue = 3029,                 // Adding Auto Build Items to the Queue
    idhConditionsThatAffectProduction = 3031,                 // Conditions that Affect Production
    idhResearch = 3032,                                       // Research
    idhFieldsOfStudy = 3033,                                  // Fields of Study
    idhAllocatingResourcesForResearch = 3034,                 // Allocating Resources for Research
    idhTheCostOfResearch = 3035,                              // The Cost of Research
    idhDesigningShips = 3037,                                 // Designing Ships
    idhHowToApproachShipDesign = 3038,                        // How to Approach Ship Design
    idhDesigningANewShipFromScratch = 3039,                   // Designing a New Ship from Scratch
    idhEditingAnExistingShipDesign = 3040,                    // Editing an Existing Ship Design
    idhDeletingAnExistingShipDesign = 3041,                   // Deleting an Existing Ship Design
    idhReachingTheMaximumNumberOfDesigns = 3042,              // Reaching the Maximum Number of Designs
    idhCountingTheNumberOfShipDesigns = 3043,                 // Counting the Number of Ship Designs
    idhAddingShipBasedScanners = 3044,                        // Adding Ship-based Scanners
    idhAddingCloakingDevices = 3045,                          // Adding Cloaking Devices
    idhEngines = 3046,                                        // Engines
    idhLearningAboutOtherPlayersHulls = 3047,                 // Learning About Other Player's Hulls
    idhManagingFleets = 3048,                                 // Managing Fleets
    idhAssemblingFleets = 3049,                               // Assembling Fleets
    idhWarpSpeed = 3050,                                      // Warp Speed
    idhFindingASingleFleet = 3051,                            // Finding a Single Fleet
    idhFindingASpecificFleetComposition = 3052,               // Finding a Specific Fleet Composition
    idhSwitchingBetweenFleets = 3053,                         // Switching Between Fleets
    idhNamingFleets = 3054,                                   // Naming Fleets
    idhUsingFuel = 3055,                                      // Using Fuel
    idhRendezvousingFleets = 3056,                            // Rendezvousing Fleets
    idhTransferringCargo = 3057,                              // Transferring Cargo
    idhJettisoningCargo = 3058,                               // Jettisoning Cargo
    idhSplittingAndMergingFleets = 3059,                      // Splitting and Merging Fleets
    idhScrappingFleets = 3060,                                // Scrapping Fleets
    idhNavigation = 3061,                                     // Navigation
    idhAddingFleetWaypoints = 3062,                           // Adding Fleet Waypoints
    idhMovingFleetWaypoints = 3063,                           // Moving Fleet Waypoints
    idhDeletingFleetWaypoints = 3064,                         // Deleting Fleet Waypoints
    idhStargateNavigation = 3065,                             // Stargate Navigation
    idhWormholeNavigation = 3066,                             // Wormhole Navigation
    idhColonization = 3068,                                   // Colonization
    idhChoosingPlanetsToColonize = 3069,                      // Choosing Planets to Colonize
    idhColonizingAnUninhabitedPlanet = 3070,                  // Colonizing an Uninhabited Planet
    idhShuttlingColonistsWithFreighters = 3071,               // Shuttling Colonists with Freighters
    idhHeyThatPlanetsAlreadyInhabited = 3072,                 // Hey, that Planet's Already Inhabited!
    idhMining = 3073,                                         // Mining
    idhMiningColonizedWorlds = 3074,                          // Mining Colonized Worlds
    idhRemoteMining3075 = 3075,                               // Remote Mining
    idhCreatingARobotMiningFleet = 3076,                      // Creating a Robot Mining Fleet
    idhTransportingFreight = 3078,                            // Transporting Freight
    idhShippingFreight = 3079,                                // Shipping Freight
    idhFlingingMassPackets = 3080,                            // Flinging Mass Packets
    idhTheBasicsOfCombat = 3081,                              // The Basics of Combat
    idhFleetToFleetCombat = 3082,                             // Fleet-to-fleet Combat
    idhBombingPlanets = 3083,                                 // Bombing Planets
    idhGroundCombat = 3084,                                   // Ground Combat
    idhLayingMinefields = 3085,                               // Laying Minefields
    idhStarbaseCombat = 3086,                                 // Starbase Combat
    idhDeclaringEnemiesAndFriends = 3087,                     // Declaring Enemies and Friends
    idhBattlePlans = 3088,                                    // Battle Plans
    idhMakingANewBattlePlan = 3089,                           // Making a New Battle Plan
    idhReviewABattleInSpace = 3092,                           // Review a Battle in Space
    idhViewingEnemyFleetsInTheSummaryPane = 3093,             // Viewing Enemy Fleets in the Summary Pane
    idhViewingEnemyShipDesigns = 3094,                        // Viewing Enemy Ship Designs
    idhPatroling = 3095,                                      // Patroling
    idhScanningAndCloaking = 3096,                            // Scanning and Cloaking
    idhSelectingFleetsInTheScannerPane = 3097,                // Selecting Fleets in the Scanner Pane
    idhScanningPlanets = 3098,                                // Scanning Planets
    idhCloakingOrHidingFromAnOpponentsScanners = 3099,        // Cloaking, or Hiding from an Opponents Scanners
    idhDetectingAnOpponentsFleets = 3100,                     // Detecting an Opponents Fleets
    idhScannerTechnology = 3102,                              // Scanner Technology
    idhCreatingACustomTransportZipOrder = 3103,               // Creating a Custom Transport Zip Order
    idhChangingTheContentsOfABattlePlan = 3105,               // Changing the Contents of a Battle Plan
    idhHowToTerraform = 3106,                                 // How to Terraform
    idhMinefields = 3107,                                     // Minefields
    idhSweepingMinefields = 3108,                             // Sweeping Minefields
    idhPiratingUsingStealthBasedScanners = 3114,              // Pirating using Stealth-based Scanners
    idhDiplomacyAndTrade = 3115,                              // Diplomacy and Trade
    idhRoutingFleets = 3116,                                  // Routing Fleets
    idhProductionTemplates = 3117,                            // Production Templates
    idhTargeting = 3118,                                      // Targeting
    idhTactics = 3119,                                        // Tactics
    idhRemoteTerraforming3120 = 3120,                         // Remote Terraforming
    idhRemotelyDetonatingMinefields = 3121,                   // Remotely Detonating Minefields
    idhSalvageFromSpaceBattles = 3122,                        // Salvage from Space Battles
    idhPlanetReports = 3123,                                  // Planet Reports
    idhChangingTheOrderOfPlanetsInTheProductionDialog = 3124, // Changing the Order of Planets in the Production dialog
    idhViewingStarsTechnology = 3125,                         // Viewing Stars! Technology
    idhReportsOnYourFleets = 3126,                            // Reports on Your Fleets
    idhJointVenturesInRemoteMining = 3127,                    // Joint Ventures in Remote Mining
    idhMineralPacketBombardment = 3128,                       // Mineral Packet Bombardment
    idhBattleReports = 3129,                                  // Battle Reports
    idhFleetReportsOnEnemiesAndOtherPlayers = 3130,           // Fleet Reports on Enemies and other Players
    idhReports = 3131,                                        // Reports
    idhPrintingAMapOfTheUniverse = 3132,                      // Printing a Map of the Universe
    idhWhatYouNeedToPlay = 3501,                              // What You Need to Play
    idhTuningStarsForYourScreenResolution = 3502,             // Tuning Stars for Your Screen Resolution
    idhStartingASinglePlayerGame = 3504,                      // Starting a Single Player Game
    idhHostingAMultiPlayerGame = 3505,                        // Hosting a Multi-Player Game
    idhWhatEachPlayerNeedsToDo = 3507,                        // What Each Player Needs to Do
    idhHostingANetworkGame = 3508,                            // Hosting a Network Game
    idhBeingAbsentFromPlay = 3511,                            // Being Absent from Play
    idhWinning = 3512,                                        // Winning
    idhOptionsForLaunchingStars = 3513,                       // Options for Launching Stars!
    idhExitingStars = 3514,                                   // Exiting Stars!
    idhCopyProtection = 3516,                                 // Copy Protection
    idhPlayingWithACustomRace = 3517,                         // Playing with a Custom Race
    idhCreatingAndSavingACustomRace = 3518,                   // Creating and Saving a Custom Race
    idhAddingAnExistingRaceToANewGame = 3520,                 // Adding an Existing Race to a New Game
    idhEditingAnExistingCustomRace = 3521,                    // Editing an Existing Custom Race
    idhSubmittingBugReports = 3523,                           // Submitting Bug Reports
    idhHostingModemAndEmailGames = 3526,                      // Hosting Modem and Email Games
    idhSavingYourGameWhatItMeans = 3528,                      // Saving Your Game--What it Means
    idhCreatingAPassword = 3529,                              // Creating a Password
    idhCreatingAUniverseFromTheCommandLine = 3531,            // Creating a Universe from the Command Line
    idhStarsWebSite = 3532,                                   // Stars! Web Site
    idhPlayingTheTutorial = 3533,                             // Playing the Tutorial
    idhReplayingAPreviousTurn = 3534,                         // Replaying a Previous Turn
    idhHostingHotSeatGames = 3535,                            // Hosting Hot-Seat Games
    idhAddingExpansionPlayers = 3538,                         // Adding Expansion Players
    idhOrderingTheRetailVersionOfStars = 4001,                // Ordering the Retail Version of Stars!
    idhWelcomeToStars = 4501,                                 // Welcome to Stars!
    idhMultiPlayerGames = 4506,                               // Multi-player games
    idhAddedCostOfResearch = 5501,                            // Added Cost of Research
    idhAIs = 5502,                                            // AIs
    idhAIsAndAdvantagePoints = 5503,                          // AIs and Advantage Points
    idhAnnualGrowthRate = 5504,                               // Annual Growth Rate
    idhBestWarpSpeed = 5505,                                  // Best Warp Speed
    idhDefensesAndInvadingTroops = 5507,                      // Defenses and Invading Troops
    idhDisengaging = 5508,                                    // Disengaging
    idhEnergySourcesForStarships = 5509,                      // Energy Sources for Starships
    idhFactory = 5510,                                        // Factory
    idhFibonacciSeries = 5511,                                // Fibonacci Series
    idhFleetColors = 5512,                                    // Fleet Colors
    idhFuelPoorPlanets = 5513,                                // Fuel Poor Planets
    idhLoadFromFleet = 5514,                                  // Load from Fleet
    idhLoadOptimal = 5515,                                    // Load Optimal
    idhLosingColonists = 5516,                                // Losing Colonists
    idhMaximumShipDesignsAndShips = 5517,                     // Maximum Ship Designs and Ships
    idhMine = 5519,                                           // Mine
    idhMineralAlchemy5520 = 5520,                             // Mineral Alchemy
    idhOrbitRingColors = 5521,                                // Orbit Ring Colors
    idhPlanetPenetratingScanners = 5522,                      // Planet penetrating scanners
    idhResources = 5523,                                      // Resources
    idhRoundOfBattle = 5524,                                  // Round of Battle
    idhShipClasses = 5527,                                    // Ship Classes
    idhToken = 5528,                                          // Token
    idhWaitForFleet = 5529,                                   // Wait for Fleet
    idhCollateralDamage = 5530,                               // Collateral Damage
    idhDefineInitiative = 5531,                               // define Initiative
    idhViewsInTheScannerPane = 5532,                          // Views in the Scanner pane
    idhBattleSpeed = 5533,                                    // Battle Speed
    idhCapitalShip = 5534,                                    // Capital Ship
    idhDialogsAndDisplays = 5535,                             // Dialogs and Displays
    idhKeyboardShortcuts = 6001,                              // Keyboard Shortcuts
    idhRating = 6501,                                         // Rating
    idhRaceDescriptionFileNameRNFiles = 6502,                 // Race Description file -- name.rN files
    idhHostFile = 6503,                                       // Host file
    idhPlayerLogFile = 6504,                                  // Player Log file
    idhRaceFile = 6505,                                       // Race file
    idhRaceFileHowToPopup = 12003,                            // Race File How to Popup
    idhHowToManageProduction = 12004,                         // How to Manage Production
    idhHowToAssignWaypoints = 12005,                          // How to Assign Waypoints
    idhHowToManageBattles = 12006,                            // How to Manage Battles
    idhHowToDesignShips = 12007,                              // How to Design Ships
    idhHowToColonize = 12008,                                 // How to Colonize
    idhPopupPlanetTiles = 12009,                              // Popup Planet Tiles
    idhPopupFleetTiles = 12010,                               // Popup Fleet Tiles
    idhPopupWaypointTasks = 12011,                            // Popup Waypoint Tasks
    idhPopupScannerTopics = 12012,                            // popup Scanner Topics
    idhPopupBattleDetails = 12015,                            // popup Battle Details
    idhPopupTables = 12017,                                   // popup Tables
    idhPlanets12018 = 12018,                                  // Planets
    idhPopupTerraforming = 12020,                             // popup Terraforming
    idhPopupOrbitalDevices = 12021,                           // popup Orbital Devices
    idhPopupProduction = 12022,                               // popup Production
    idhPopupResearch = 12023,                                 // popup Research
    idhPopupShipDesign = 12024,                               // popup Ship Design
    idhPopupFleetManagement = 12025,                          // popup Fleet Management
    idhPopupNavigation = 12026,                               // popup Navigation
    idhPopupColonization = 12027,                             // popup Colonization
    idhPopupMining = 12028,                                   // popup Mining
    idhPopupFreight = 12029,                                  // popup Freight
    idhPopupCombat = 12030,                                   // popup Combat
    idhPopupScanning = 12033,                                 // popup Scanning
    idhMenuCustomRaceWizardSteps = 12040,                     // menu Custom Race Wizard Steps
    idhMenuAdvancedSetup = 12041,                             // menu Advanced Setup
    idhHowToManageFleets = 12042,                             // How to Manage Fleets
    idhScannerViews = 12043,                                  // Scanner Views
    idhPopupResDialog = 12046,                                // popup res dialog
    idhPopupMinefields = 12048,                               // popup Minefields
    idhPopupBattlePlans = 12049,                              // popup Battle Plans
    idhPopupMessagesPane = 12050,                             // popup Messages Pane
    idhHostingMultiPlayerGames = 12051,                       // Hosting Multi-Player Games
    idhMessagesPane = 14001,                                  // Messages Pane
    idhTheGotoPreviousAndNextButtons = 14002,                 // The Goto, Previous and Next Buttons
    idhSendingMessagesToOtherPlayers = 14003,                 // Sending Messages to other Players
    idhFilteredMessageCheckbox = 14004,                       // Filtered Message Checkbox
    idhFilteringMessageTypes = 14005,                         // Filtering Message Types
    idhScannerPane = 14006,                                   // Scanner Pane
    idhChoosingYourViewOfTheUniverse = 14008,                 // Choosing Your View of the Universe
    idhNormalView = 14009,                                    // Normal View
    idhPlanetValueView = 14010,                               // Planet Value View
    idhMineralsAtPlanetView = 14011,                          // Minerals at Planet View
    idhPopulationView = 14012,                                // Population View
    idhNoPlayerInformationView = 14013,                       // No Player Information View
    idhAddWaypointsOverlay = 14014,                           // Add Waypoints Overlay
    idhRadarOverlay = 14015,                                  // Radar Overlay
    idhFleetOverlay = 14016,                                  // Fleet Overlay
    idhShipFilterOverlay = 14017,                             // Ship Filter Overlay
    idhPlanetNamesOverlay = 14018,                            // Planet Names Overlay
    idhStatusBar = 14019,                                     // Status Bar
    idhZooming = 14022,                                       // Zooming
    idhSelectionSummaryPane = 14023,                          // Selection Summary pane
    idhPlanetSummary = 14024,                                 // Planet Summary
    idhMultipleObjectsIndicator = 14025,                      // Multiple Objects Indicator
    idhReportVintage = 14026,                                 // Report Vintage
    idhPopulationStatus = 14027,                              // Population Status
    idhSelectionValue = 14028,                                // Selection Value
    idhStarbaseIndicator = 14029,                             // Starbase Indicator
    idhEnvironmentGraph = 14030,                              // Environment Graph
    idhMineralContentGraph = 14031,                           // Mineral Content Graph
    idhFleetSummary = 14032,                                  // Fleet Summary
    idhMineFieldsOverlay = 14033,                             // Mine Fields overlay
    idhKeyToTheScanner = 14034,                               // Key to the Scanner
    idhIdleFleetsOverlay = 14035,                             // Idle Fleets Overlay
    idhQuickReferenceToScannerUsage = 14036,                  // Quick Reference to Scanner Usage
    idhFilterEnemyShipsOverlay = 14037,                       // Filter Enemy Ships overlay
    idhMineralConcentrationView = 14038,                      // Mineral Concentration view
    idhShipCountOverlay = 14039,                              // Ship Count overlay
    idhRainbowEffect = 14040,                                 // Rainbow Effect
    idhDisplayingPlayerColors = 14041,                        // Displaying Player Colors
    idhTroubleshootingWhatToDoWhenTheShipHitsTheFan = 52224,  // Troubleshooting: What to Do when the Ship Hits the Fan
};
typedef uint16_t HelpContextId;

// HullCategory is a hull's role, as the fleet and battle reports group
// ships: unarmed hulls are colony ships, freighters, miners and fuel
// transports.
enum HullCategory {
    hullCatColony = 0,
    hullCatFreighter = 1,
    hullCatScout = 2,   // scout, frigate, destroyer
    hullCatWarship = 3, // cruiser through dreadnought
    hullCatUtility = 4, // privateer, rogue, galleon, mine layers, Nubian, morphs
    hullCatBomber = 5,
    hullCatMiner = 6,
    hullCatFuelTransport = 7,
};
typedef uint16_t HullCategory;

// HullAttack is how a hull fights; battle code treats any nonzero value as armed.
enum HullAttack {
    hullAttackNone = 0,
    hullAttackLight = 1, // scout, frigate, destroyer, privateer
    hullAttackHeavy = 2, // cruisers and up, rogue, galleon, Nubian, morphs
    hullAttackBomber = 3,
};
typedef uint16_t HullAttack;

// BeamAbility is a beam weapon's grfAbilities.
enum BeamAbility {
    beamSapper = 0x0001,  // damages shields only
    beamGatling = 0x0002, // hits every target in range
};
typedef uint16_t BeamAbility;

// EngineAbility is a special engine's grfAbilities, which selects its
// description and restrictions.
enum EngineAbility {
    engineAbilityNone = 0,
    engineSettlersDelight = 1,   // mini-colonizer hulls only
    engineRadiatingRamScoop = 2, // radiation kills colonists
    engineFuelMizer = 3,         // requires Improved Fuel Efficiency
    engineGalaxyScoop = 4,       // requires Improved Fuel Efficiency
    engineInterspace10 = 5,      // unavailable with No Ram Scoop Engines
    engineEnigmaPulsar = 6,      // origin unknown
};
typedef uint16_t EngineAbility;

// ScannerAbility is a scanner's grfAbilities.
enum ScannerAbility {
    scannerAbilityNone = 0,
    scannerPenetrating50 = 1,
    scannerPenetrating100 = 2,
    scannerPenetrating200 = 3,
    scannerSteals = 4, // Super Stealth mineral thieves
};
typedef uint16_t ScannerAbility;

// PaneSplitter is the set of frame splitter bars a point is on.
enum PaneSplitter {
    splitVertical = 0x0001, // between the left panes and the scanner
    splitMessages = 0x0002, // below the messages pane
    splitLower = 0x0004,    // the second horizontal bar
};
typedef uint16_t PaneSplitter;

// RaceWizardPage is the race wizard page shown, in wizard order.
enum RaceWizardPage {
    rwPageNone = -1,
    rwPageRace = 1,         // IDD_RACE_WIZARD_1
    rwPagePrimaryTrait = 2, // IDD_RACE_WIZARD_4
    rwPageLesserTraits = 3, // IDD_RACE_WIZARD_5
    rwPageHabitability = 4, // IDD_RACE_WIZARD_2
    rwPageEconomy = 5,      // IDD_RACE_WIZARD_3
    rwPageResearch = 6,     // IDD_RACE_WIZARD_6
};
typedef int16_t RaceWizardPage;

// WizardButton is a wizard page's result: the index of the button pressed
// in rgidRaceBtn.
enum WizardButton {
    wizCancel = 0,
    wizBack = 1,
    wizNext = 2,
    wizFinish = 3,
    wizHelp = 4,
};
typedef uint16_t WizardButton;

// PacketDecay is how many warps a mineral packet was flung over its
// driver's rating, and so how fast it decays.
enum PacketDecay {
    decayNone = 0,
    decay10Pct = 1,
    decay25Pct = 2,
    decay50Pct = 3,
};
typedef uint16_t PacketDecay;

// CompassDir is a direction on the map, counterclockwise from east; y
// grows southward.
enum CompassDir {
    dirEast = 0,
    dirNorthEast = 1,
    dirNorth = 2,
    dirNorthWest = 3,
    dirWest = 4,
    dirSouthWest = 5,
    dirSouth = 6,
    dirSouthEast = 7,
};
typedef uint16_t CompassDir;

#endif
