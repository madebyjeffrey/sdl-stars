// Exact wire expectations come from S01's little-endian field table, not from
// a native struct dump. Exercise unaligned slices and every signed 16-bit value.
#include "recordcodec.h"
#include "battlecodec.h"
#include "battleviews.h"
#include "aiviews.h"
#include "messageviews.h"
#include "logviews.h"
#include "streamviews.h"
#include <array>
#include <cstdio>
#include <limits>
#include <new>

#ifdef STARS_TEST_NATURAL_RECORD_LAYOUT
static_assert(alignof(RTBOF) > 1);
static_assert(offsetof(SHDEF, cBuilt) != 0x7f);
#endif

#define CHECK(f) do { if (!(f)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

int main() {
    for (std::uint32_t w = 0; w < 65536; w++) {
        PLAYER plr{};
        MDPLR md{};
        PRODQ1 pq{};
        plr.SetWMdPlr(static_cast<std::uint16_t>(w)); plr.SetWFlags(static_cast<std::uint16_t>(w)); md.SetWMdPlr(static_cast<std::uint16_t>(w));
        CHECK(plr.wMdPlr() == w && plr.mdPlayer() == (w >> 9) && plr.reserved() == (w & 511) && md.wMdPlr() == w);
        CHECK(plr.det == (w & 7) && plr.iPlrBmp == ((w >> 3) & 31) && plr.fInclude == ((w >> 8) & 1));
        CHECK(plr.fAi == ((w >> 9) & 1) && plr.lvlAi == ((w >> 10) & 7) && plr.idAi == (w >> 13));
        CHECK(md.reserved == (w & 511) && md.fAi == plr.fAi && md.lvlAi == plr.lvlAi && md.idAi == plr.idAi);
        CHECK(plr.wFlags() == w && plr.fDead == (w & 1) && plr.fCrippled == ((w >> 1) & 1));
        CHECK(plr.fUnused == ((w >> 2) & 1) && plr.fLearned == ((w >> 3) & 1) && plr.fHacker == ((w >> 4) & 1) && plr.unused == (w >> 5));
        pq.w = static_cast<std::uint16_t>(w);
        CHECK(pq.mdIdle() == (w & 63) && pq.cQuan() == (w >> 6));
        pq.SetMdIdle(static_cast<std::uint16_t>(65535 - w));
        CHECK(pq.mdIdle() == ((65535 - w) & 63) && pq.cQuan() == (w >> 6));
        pq.SetCQuan(static_cast<std::uint16_t>(w));
        CHECK(pq.mdIdle() == ((65535 - w) & 63) && pq.cQuan() == (w & 1023));
        plr.iTechCur = std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(w));
        const std::int16_t hi = (w & 255) >> 4;
        CHECK(plr.iTechNow() == (w & 15) && plr.iTechNext() == (hi < 8 ? hi : hi - 16));
        plr.SetITechNow(static_cast<std::uint8_t>(65535 - w));
        CHECK(plr.iTechNow() == ((65535 - w) & 15) && plr.iTechNext() == (hi < 8 ? hi : hi - 16));
        plr.SetITechNext(static_cast<std::int8_t>(w & 127));
        const std::int16_t lo = w & 15;
        CHECK(plr.iTechNext() == (lo < 8 ? lo : lo - 16) && plr.iTechNow() == ((65535 - w) & 15));
    }
    {
        std::array<std::byte, 192> golden;
        for (std::size_t i = 0; i < golden.size(); i++) golden[i] = static_cast<std::byte>((i * 73 + 11) & 255);
        auto b = [&](std::size_t i) { return std::to_integer<std::uint32_t>(golden[i]); };
        auto w = [&](std::size_t i) { return b(i) | (b(i + 1) << 8); };
        auto l = [&](std::size_t i) { return w(i) | (w(i + 2) << 16); };
        auto si = [&](std::size_t i) { return w(i) < 32768 ? std::int32_t(w(i)) : std::int32_t(w(i)) - 65536; };
        PLAYER plr{};
        CHECK(FDecodePlayer(golden, plr));
        CHECK(static_cast<std::uint8_t>(plr.iPlayer) == b(0) && static_cast<std::uint8_t>(plr.cShDef) == b(1));
        CHECK(plr.cPlanet == si(2) && plr.cFleet == (w(4) & 4095) && plr.cshdefSB == (w(4) >> 12));
        CHECK(plr.wMdPlr() == w(6) && plr.idPlanetHome == si(8) && plr.wScore == w(10) && std::uint32_t(plr.lSalt) == l(12));
        CHECK(!std::memcmp(plr.rgEnvVar, golden.data() + 16, 3) && !std::memcmp(plr.rgEnvVarMin, golden.data() + 19, 3));
        CHECK(!std::memcmp(plr.rgEnvVarMax, golden.data() + 22, 3) && std::uint8_t(plr.pctIdealGrowth) == b(25));
        CHECK(!std::memcmp(plr.rgTech, golden.data() + 26, 6));
        for (int i = 0; i < 6; i++) CHECK(plr.rgResSpent[i] == l(32 + i * 4));
        CHECK(std::uint8_t(plr.pctResearch) == b(56) && std::uint8_t(plr.iTechCur) == b(57) && std::uint32_t(plr.lResLastYear) == l(58));
        CHECK(!std::memcmp(plr.rgAttr, golden.data() + 62, 16) && std::uint32_t(plr.grbitAttr) == l(78) && std::uint16_t(plr.grbitTrader) == w(82));
        CHECK(plr.wFlags() == w(84) && plr.zpq1.fNoResearch == b(86) && plr.zpq1.cpq == b(87));
        for (int i = 0; i < 12; i++) CHECK(plr.zpq1.rgpq[i].w == w(88 + i * 2));
        CHECK(!std::memcmp(plr.rgmdRelation, golden.data() + 112, 16));
        CHECK(!std::memcmp(plr.szName, golden.data() + 128, 32) && !std::memcmp(plr.szNames, golden.data() + 160, 32));
        std::array<std::byte, 194> rgb;
        rgb.fill(std::byte{0xa5});
        CHECK(FEncodePlayer(plr, std::span(rgb).subspan(1, 192)) && !std::memcmp(rgb.data() + 1, golden.data(), 192));
        CHECK(rgb.front() == std::byte{0xa5} && rgb.back() == std::byte{0xa5});
        for (std::size_t cb = 0; cb < 192; cb++) {
            PLAYER untouched{}; untouched.iPlayer = 123;
            CHECK(!FDecodePlayer(std::span(golden).first(cb), untouched) && untouched.iPlayer == 123);
            rgb.fill(std::byte{0xa5});
            CHECK(!FEncodePlayer(plr, std::span(rgb).subspan(1, cb)));
            for (auto ch : rgb) CHECK(ch == std::byte{0xa5});
        }
        plr.idPlanetHome = 123; plr.zpq1.cpq = 42; plr.rgmdRelation[0] = 99; plr.szName[0] = 'z';
        CHECK(FDecodePlayer(std::span(golden).first(8), plr, 8));
        CHECK(plr.idPlanetHome == 123 && plr.zpq1.cpq == 42 && plr.rgmdRelation[0] == 99 && plr.szName[0] == 'z');
        CHECK(FDecodePlayer(std::span(golden).first(112), plr, 112));
        CHECK(plr.idPlanetHome == si(8) && plr.zpq1.cpq == b(87) && plr.rgmdRelation[0] == 99 && plr.szName[0] == 'z');
        if constexpr (alignof(PLAYER) == 1) {
            alignas(4) std::byte rgbPlr[sizeof(PLAYER) + 1]{};
            auto *odd = ::new (rgbPlr + 1) PLAYER{};
            CHECK(FDecodePlayer(golden, *odd) && FEncodePlayer(*odd, std::span(rgb).subspan(1, 192)));
            CHECK(!std::memcmp(rgb.data() + 1, golden.data(), 192));
        }
    }

    for (std::uint32_t w = 0; w < 65536; w++) {
        INI ini{};
        TUTOR tutor{};
        GDATA gd{};
        gd.fUnused2 = 12345;
        ini.SetWFlags(static_cast<std::uint16_t>(w));
        CHECK(ini.wFlags() == w && ini.fStartupFile == (w & 1) && ini.grobjSel == ((w >> 5) & 15));
        CHECK(ini.fCmdLine == ((w >> 1) & 1) && ini.fWait == ((w >> 2) & 1) && ini.fGen == ((w >> 3) & 1));
        CHECK(ini.fTry == ((w >> 4) & 1) && ini.fBatch == ((w >> 9) & 1) && ini.fNewGame == ((w >> 10) & 1));
        CHECK(ini.fDumpFleets == ((w >> 11) & 1) && ini.fDumpPlanets == ((w >> 12) & 1) && ini.fDumpMap == ((w >> 13) & 1));
        CHECK(ini.fValidate == ((w >> 14) & 1) && ini.fLogging == (w >> 15));
        const auto flags = static_cast<std::int16_t>(w < 32768 ? std::int32_t(w) : std::int32_t(w) - 65536);
        tutor.SetWFlags(flags);
        CHECK(tutor.wFlags() == flags && tutor.cError == ((w >> 6) & 7) && tutor.unused == (w >> 15));
        CHECK(tutor.fVisible == (w & 1) && tutor.fGameSaved == ((w >> 1) & 1) && tutor.fChange == ((w >> 2) & 1));
        CHECK(tutor.fTurnDone == ((w >> 3) & 1) && tutor.fTutorDone == ((w >> 4) & 1) && tutor.fNoErrors == ((w >> 5) & 1));
        CHECK(tutor.fAutoComplete == ((w >> 9) & 1) && tutor.fProgress == ((w >> 10) & 1) && tutor.fTBVis == ((w >> 11) & 1));
        CHECK(tutor.fValidQ == ((w >> 12) & 1) && tutor.fFreeing == ((w >> 13) & 1) && tutor.fShowHidMsg == ((w >> 14) & 1));
        const std::uint32_t bits = w | ((65535u - w) << 16);
        const auto signedBits = std::bit_cast<std::int32_t>(bits);
        gd.SetGrBits(signedBits); gd.SetGrBits2(signedBits);
        CHECK(gd.grBits() == signedBits && gd.grBits2() == signedBits);
        CHECK(gd.fUnknownShip == ((bits >> 0) & 1u));
        CHECK(gd.fGeneratingTurn == ((bits >> 1) & 1u));
        CHECK(gd.fForceTurn == ((bits >> 2) & 1u));
        CHECK(gd.fHostMode == ((bits >> 3) & 1u));
        CHECK(gd.fSubmit == ((bits >> 4) & 1u));
        CHECK(gd.fNoResearchSav == ((bits >> 5) & 1u));
        CHECK(gd.fRadiatingEngine == ((bits >> 6) & 1u));
        CHECK(gd.fNoIdleChecks == ((bits >> 7) & 1u));
        CHECK(gd.fSendMsgMode == ((bits >> 8) & 1u));
        CHECK(gd.fRetryOpens == ((bits >> 9) & 1u));
        CHECK(gd.fAisDone == ((bits >> 10) & 1u));
        CHECK(gd.fTutorial == ((bits >> 11) & 1u));
        CHECK(gd.fGotoVCR == ((bits >> 12) & 1u));
        CHECK(gd.fVCRTimer == ((bits >> 13) & 1u));
        CHECK(gd.mdScreenSize == ((bits >> 14) & 3u));
        CHECK(gd.fGameOverMan == ((bits >> 16) & 1u));
        CHECK(gd.fDontDoLogFiles == ((bits >> 17) & 1u));
        CHECK(gd.fFileCrippled == ((bits >> 18) & 1u));
        CHECK(gd.fSmallTileMode == ((bits >> 19) & 1u));
        CHECK(gd.fAllAis == ((bits >> 20) & 1u));
        CHECK(gd.fReadOnly == ((bits >> 21) & 1u));
        CHECK(gd.fExitWindows == ((bits >> 22) & 1u));
        CHECK(gd.fPartialTurn == ((bits >> 23) & 1u));
        CHECK(gd.fSetMassMode == ((bits >> 24) & 1u));
        CHECK(gd.fRptSafeDraw == ((bits >> 25) & 1u));
        CHECK(gd.fProgressTxt == ((bits >> 26) & 1u));
        CHECK(gd.fSoundFX == ((bits >> 27) & 1u));
        CHECK(gd.fNoSound == ((bits >> 28) & 1u));
        CHECK(gd.fSetRouteMode == ((bits >> 29) & 1u));
        CHECK(gd.fBleedingEdge == ((bits >> 30) & 1u));
        CHECK(gd.fToolbar == ((bits >> 31) & 1u));
        CHECK(gd.fNoScannerDraw == ((bits >> 0) & 1u));
        CHECK(gd.fTrialPeriodOver == ((bits >> 1) & 1u));
        CHECK(gd.fClose == ((bits >> 2) & 1u));
        CHECK(gd.fDontCalcBleed == ((bits >> 3) & 1u));
        CHECK(gd.fChgZipOrd == ((bits >> 4) & 1u));
        CHECK(gd.fChgZipProd == ((bits >> 5) & 1u));
        CHECK(gd.fChgScanner == ((bits >> 6) & 1u));
        CHECK(gd.fChgReports == ((bits >> 7) & 1u));
        CHECK(gd.fWriteTurnNum == ((bits >> 8) & 1u));
        CHECK(gd.fHotSeat == ((bits >> 9) & 1u));
        CHECK(gd.fFleetLinkValid == ((bits >> 10) & 1u));
        CHECK(gd.fScoreVictory == ((bits >> 11) & 3u));
        CHECK(gd.iCurGraph == ((bits >> 16) & 15u));
        CHECK(gd.fMusic == ((bits >> 20) & 1u));
        CHECK(gd.fPerPlayerDumps == ((bits >> 21) & 1u));
        CHECK(gd.fNoHostNames == ((bits >> 22) & 1u));
        CHECK(gd.fUnused2 == 12345);
        CHECK(gd.unused3 == ((bits >> 13) & 7) && gd.unused4 == (bits >> 23));
        ini.SetWFlags(0); tutor.SetWFlags(0); gd.SetGrBits(0); gd.SetGrBits2(0);
        CHECK(ini.wFlags() == 0 && tutor.wFlags() == 0 && gd.grBits() == 0 && gd.grBits2() == 0 && gd.fUnused2 == 12345);
    }
    if constexpr (alignof(GDATA) == 1 && alignof(TUTOR) == 1) {
        alignas(4) std::byte rgbGd[sizeof(GDATA) + 1]{}, rgbTutor[sizeof(TUTOR) + 1]{};
        auto *gd = ::new (rgbGd + 1) GDATA{};
        auto *tutor = ::new (rgbTutor + 1) TUTOR{};
        gd->SetGrBits(-1); gd->SetGrBits2(-2); tutor->SetWFlags(-3);
        CHECK(gd->grBits() == -1 && gd->grBits2() == -2 && tutor->wFlags() == -3);
    }

    for (std::uint32_t w = 0; w < 65536; w++) {
        const std::uint32_t flags = 65535 - w;
        const std::array<std::byte, 4> golden = {static_cast<std::byte>(w), static_cast<std::byte>(w >> 8), static_cast<std::byte>(flags), static_cast<std::byte>(flags >> 8)};
        RTPLANET rtpl{};
        std::array<std::byte, 6> rgb;
        rgb.fill(std::byte{0xa5});
        const std::int16_t id = static_cast<std::int16_t>((w & 2047) < 1024 ? std::int32_t(w & 2047) : std::int32_t(w & 2047) - 2048);
        const std::int16_t player = static_cast<std::int16_t>((w >> 11) < 16 ? std::int32_t(w >> 11) : std::int32_t(w >> 11) - 32);
        CHECK(FDecodePlanetPrefix(golden, rtpl) && rtpl.id == id && rtpl.iPlayer == player && rtpl.det == (flags & 127));
        CHECK(FEncodePlanetPrefix(rtpl, std::span(rgb).subspan(1, 4)) && !std::memcmp(rgb.data() + 1, golden.data(), 4));
        CHECK(rgb.front() == std::byte{0xa5} && rgb.back() == std::byte{0xa5});
        auto view = PpvFromPrefix(rgb.data() + 1, 4);
        CHECK(view.id() == id && view.iPlayer() == player && view.det() == (flags & 127));
        CHECK(view.fHomeworld() == ((flags >> 7) & 1) && view.fInclude() == ((flags >> 8) & 1) && view.fStarbase() == ((flags >> 9) & 1));
        CHECK(view.fIncEVO() == ((flags >> 10) & 1) && view.fIncImp() == ((flags >> 11) & 1) && view.fIsArtifact() == ((flags >> 12) & 1));
        CHECK(view.fIncSurfMin() == ((flags >> 13) & 1) && view.fRouting() == ((flags >> 14) & 1) && view.fFirstYear() == (flags >> 15));
        view.SetId(id); view.SetIPlayer(player); view.SetDet(rtpl.det); view.SetFHomeworld(rtpl.fHomeworld); view.SetFInclude(rtpl.fInclude);
        view.SetFStarbase(rtpl.fStarbase); view.SetFIncEVO(rtpl.fIncEVO); view.SetFIncImp(rtpl.fIncImp); view.SetFIsArtifact(rtpl.fIsArtifact);
        view.SetFIncSurfMin(rtpl.fIncSurfMin); view.SetFRouting(rtpl.fRouting); view.SetFFirstYear(rtpl.fFirstYear);
        CHECK(!std::memcmp(rgb.data() + 1, golden.data(), 4));
        PLANET pl{};
        pl.SetUGuesses(static_cast<std::uint16_t>(w)); pl.SetWRouting(static_cast<std::uint16_t>(flags));
        CHECK(pl.uGuesses() == w && pl.uPopGuess == (w & 4095) && pl.uDefGuess == w / 4096);
        CHECK(pl.wRouting() == flags && pl.idRoute == (flags & 1023) && pl.unused4 == flags / 1024);
        const std::uint32_t ul = w * 2654435761u;
        const std::uint32_t ul2 = ~ul;
        std::array<std::uint8_t, 8> improvements = {std::uint8_t(ul), std::uint8_t(ul >> 8), std::uint8_t(ul >> 16), std::uint8_t(ul >> 24),
            std::uint8_t(ul2), std::uint8_t(ul2 >> 8), std::uint8_t(ul2 >> 16), std::uint8_t(ul2 >> 24)};
        pl.SetRgbImp(improvements);
        CHECK(pl.rgbImp() == improvements && pl.iDeltaPop == (ul & 255) && pl.cMines == ((ul >> 8) & 4095) && pl.cFactories == ul >> 20);
        CHECK(pl.cDefenses == (ul2 & 4095) && pl.iScanner == ((ul2 >> 12) & 31) && pl.unused5 == ((ul2 >> 17) & 31));
        CHECK(pl.fArtifact == ((ul2 >> 22) & 1) && pl.fNoResearch == ((ul2 >> 23) & 1) && pl.unused2 == ul2 >> 24);
        const auto l = std::bit_cast<std::int32_t>(ul);
        pl.SetLStarbase(l);
        CHECK(pl.lStarbase() == l && pl.isb == (ul & 15) && pl.pctDp == ((ul >> 4) & 4095));
        CHECK(pl.idFling == ((ul >> 16) & 1023) && pl.iWarpFling == ((ul >> 26) & 15) && pl.fNoHeal == ((ul >> 30) & 1) && pl.unused3 == ul >> 31);
        if (w == 32768) for (std::size_t cb = 0; cb < 4; cb++) {
            rtpl.id = 123; rtpl.iPlayer = -7;
            CHECK(!FDecodePlanetPrefix(std::span(golden).first(cb), rtpl) && rtpl.id == 123 && rtpl.iPlayer == -7);
            rgb.fill(std::byte{0xa5});
            CHECK(!FEncodePlanetPrefix(rtpl, std::span(rgb).subspan(1, cb)));
            for (auto ch : rgb) CHECK(ch == std::byte{0xa5});
        }
    }
    if constexpr (alignof(PLANET) == 1) {
        alignas(4) std::byte rgb[sizeof(PLANET) + 2]{};
        auto *pl = ::new (rgb + 1) PLANET{};
        pl->SetLStarbase(-2); pl->SetUGuesses(65535); pl->SetWRouting(65535);
        CHECK(pl->lStarbase() == -2 && pl->uGuesses() == 65535 && pl->wRouting() == 65535);
    }

    for (std::uint32_t i = 0; i < 65570; i++) {
        const std::uint32_t ul = i < 32 ? (std::uint32_t{1} << i) : i == 32 ? 0 : i == 33 ? 0xffffffffu : (i - 34) * 2654435761u;
        const std::array<std::byte, 6> golden = {std::byte{0}, std::byte{0x80}, static_cast<std::byte>(ul), static_cast<std::byte>(ul >> 8), static_cast<std::byte>(ul >> 16), static_cast<std::byte>(ul >> 24)};
        RTCHGPLANETLONG routing{};
        std::array<std::byte, 8> rgb;
        rgb.fill(std::byte{0xa5});
        CHECK(FDecodePlanetRouting(golden, routing) && routing.id == -32768 && routing.ul() == ul);
        CHECK(routing.fNoResearch == (ul & 1) && routing.idFling == ((ul >> 1) & 1023) && routing.iWarpFling == ((ul >> 11) & 15));
        CHECK(routing.idRoute == ((ul >> 15) & 1023) && routing.unused == ul >> 25);
        CHECK(FEncodePlanetRouting(routing, std::span(rgb).subspan(1, 6)) && !std::memcmp(rgb.data() + 1, golden.data(), 6));
        CHECK(rgb.front() == std::byte{0xa5} && rgb.back() == std::byte{0xa5});
        auto lp = rgb.data() + 1;
        CHECK(RlvFromPrefix(lp, 2).id() == -32768 && RlvFromPrefix(lp, 3).fNoResearch() == (ul & 1));
        CHECK(RlvFromPrefix(lp, 4).idFling() == ((ul >> 1) & 1023) && RlvFromPrefix(lp, 4).iWarpFling() == ((ul >> 11) & 15));
        CHECK(RlvFromPrefix(lp, 6).idRoute() == ((ul >> 15) & 1023));
        if (i == 32) for (std::size_t cb = 0; cb < 6; cb++) {
            routing.id = 123; routing.unused = 99;
            CHECK(!FDecodePlanetRouting(std::span(golden).first(cb), routing) && routing.id == 123 && routing.unused == 99);
            rgb.fill(std::byte{0xa5});
            CHECK(!FEncodePlanetRouting(routing, std::span(rgb).subspan(1, cb)));
            for (auto ch : rgb) CHECK(ch == std::byte{0xa5});
        }
    }
    for (std::uint32_t w = 0; w < 65536; w++) {
        const std::array<std::byte, 4> golden = {std::byte{0xff}, std::byte{0x80}, static_cast<std::byte>(w), static_cast<std::byte>(w >> 8)};
        RTLOGTHING thing{};
        std::array<std::byte, 6> rgb;
        rgb.fill(std::byte{0xa5});
        CHECK(FDecodeLogThing(golden, thing) && thing.idFull == 0x80ff && thing.fDetonate == (w < 32768 ? std::int32_t(w) : std::int32_t(w) - 65536));
        CHECK(FEncodeLogThing(thing, std::span(rgb).subspan(1, 4)) && !std::memcmp(rgb.data() + 1, golden.data(), 4));
        CHECK(rgb.front() == std::byte{0xa5} && rgb.back() == std::byte{0xa5});
        CHECK(LtvFromPrefix(rgb.data() + 1, 2).idFull() == 0x80ff && LtvFromPrefix(rgb.data() + 1, 4).fDetonate() == thing.fDetonate);
        if (w == 32768) for (std::size_t cb = 0; cb < 4; cb++) {
            thing.idFull = 123; thing.fDetonate = -456;
            CHECK(!FDecodeLogThing(std::span(golden).first(cb), thing) && thing.idFull == 123 && thing.fDetonate == -456);
            rgb.fill(std::byte{0xa5});
            CHECK(!FEncodeLogThing(thing, std::span(rgb).subspan(1, cb)));
            for (auto ch : rgb) CHECK(ch == std::byte{0xa5});
        }
    }

    if constexpr (alignof(SCOREX) == 1) {
        // A packed object can legally start at an odd byte. Its numeric
        // accessor must copy the field before binding a scalar reference.
        alignas(2) std::byte rgb[sizeof(SCOREX) + 2]{};
        auto *psx = ::new (rgb + 1) SCOREX{};
        psx->SetIRank(-2);
        CHECK(psx->iRank() == -2 && psx->turn == 65534);
    }

    for (std::uint32_t w = 0; w < 65536; w++) {
        const std::uint32_t turn = 65535 - w;
        const std::array<std::byte, 24> golden = {static_cast<std::byte>(w), static_cast<std::byte>(w >> 8),
            static_cast<std::byte>(turn), static_cast<std::byte>(turn >> 8), std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0x80},
            std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0x7f}, std::byte{0}, std::byte{0x80}, std::byte{0xff}, std::byte{0x7f},
            std::byte{0xff}, std::byte{0xff}, std::byte{0}, std::byte{0x80}, std::byte{1}, std::byte{0}, std::byte{0xfe}, std::byte{0xff}};
        SCOREX sx{};
        std::array<std::byte, 26> rgb;
        rgb.fill(std::byte{0xa5});
        CHECK(FDecodeScoreX(golden, sx) && sx.wWord() == w && sx.turn == turn);
        CHECK(sx.iPlayer == (w & 31) && sx.fValid == ((w >> 5) & 1) && sx.grbitVC == ((w >> 6) & 255));
        CHECK(sx.fWinner == ((w >> 14) & 1) && sx.fHistory == (w >> 15));
        CHECK(sx.iRank() == (turn < 32768 ? std::int32_t(turn) : std::int32_t(turn) - 65536));
        CHECK(sx.score.lScore == std::numeric_limits<std::int32_t>::min() && sx.score.cResources == 2147483647);
        CHECK(sx.score.cPlanet == -32768 && sx.score.cStarbase == 32767 && sx.score.cTechLevels == -2);
        CHECK(sx.score.rgcsh[0] == 65535 && sx.score.rgcsh[1] == 32768 && sx.score.rgcsh[2] == 1);
        CHECK(FEncodeScoreX(sx, std::span(rgb).subspan(1, 24)) && !std::memcmp(rgb.data() + 1, golden.data(), 24));
        CHECK(rgb.front() == std::byte{0xa5} && rgb.back() == std::byte{0xa5});
        if (w == 32768) for (std::size_t cb = 0; cb < 24; cb++) {
            sx.turn = 123; sx.score.lScore = 456;
            CHECK(!FDecodeScoreX(std::span(golden).first(cb), sx) && sx.turn == 123 && sx.score.lScore == 456);
            rgb.fill(std::byte{0xa5});
            CHECK(!FEncodeScoreX(sx, std::span(rgb).subspan(1, cb)));
            for (auto ch : rgb) CHECK(ch == std::byte{0xa5});
        }
        sx.SetIRank(static_cast<std::int16_t>(turn < 32768 ? std::int32_t(turn) : std::int32_t(turn) - 65536));
        CHECK(sx.turn == turn);
    }

    for (std::uint32_t w = 0; w < 65536; w++) {
        std::array<std::byte, 64> golden = {std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
            std::byte{1}, std::byte{0x80}, std::byte{0xff}, std::byte{0xff}, std::byte{0}, std::byte{0x80},
            std::byte{0xff}, std::byte{0x7f}, std::byte{0xfe}, std::byte{0xff}, std::byte{0xff}, std::byte{0x7f},
            static_cast<std::byte>(w % 256), static_cast<std::byte>(w / 256), std::byte{0x85}, std::byte{0xa5}};
        for (int i = 0; i < 12; i++) golden[20 + i] = static_cast<std::byte>(i + 128);
        for (int i = 0; i < 32; i++) golden[32 + i] = static_cast<std::byte>(i + 160);
        GAME game{};
        std::array<std::byte, 66> rgb;
        rgb.fill(std::byte{0xa5});
        CHECK(FDecodeGame(golden, game) && game.lid == -1 && game.mdSize == 0x8001 && game.mdDensity == 65535 && game.mdStartDist == 65534);
        CHECK(game.cPlayer == -32768 && game.cPlanMax == 32767 && game.fDirty == 32767 && game.turn == 0xa585 && game.wCrap() == w);
        CHECK(game.fExtraFuel == (w & 1) && game.fSlowTech == ((w >> 1) & 1) && game.fSinglePlr == ((w >> 2) & 1));
        CHECK(game.fTutorial == ((w >> 3) & 1) && game.fAisBand == ((w >> 4) & 1) && game.fBBSPlay == ((w >> 5) & 1));
        CHECK(game.fVisScores == ((w >> 6) & 1) && game.fNoRandom == ((w >> 7) & 1) && game.fClumping == ((w >> 8) & 1));
        CHECK(game.wGen == ((w >> 9) & 7) && game.unused == w / 4096);
        CHECK(!std::memcmp(game.rgvc, golden.data() + 20, 12) && !std::memcmp(game.szName, golden.data() + 32, 32));
        CHECK(FEncodeGame(game, std::span(rgb).subspan(1, 64)) && !std::memcmp(rgb.data() + 1, golden.data(), 64));
        CHECK(rgb.front() == std::byte{0xa5} && rgb.back() == std::byte{0xa5});
        if (w == 32768) for (std::size_t cb = 0; cb < 64; cb++) {
            game.lid = 999; game.cPlayer = -123;
            CHECK(!FDecodeGame(std::span(golden).first(cb), game) && game.lid == 999 && game.cPlayer == -123);
            rgb.fill(std::byte{0xa5});
            CHECK(!FEncodeGame(game, std::span(rgb).subspan(1, cb)));
            for (auto ch : rgb) CHECK(ch == std::byte{0xa5});
        }
    }
    for (std::uint32_t i = 0; i < 65570; i++) {
        const std::uint32_t w = i < 32 ? (std::uint32_t{1} << i) : i == 32 ? 0 : i == 33 ? 0xffffffffu : ((i - 34) | ((65569 - i) << 16));
        const std::array<std::byte, 4> golden = {static_cast<std::byte>(w), static_cast<std::byte>(w >> 8), static_cast<std::byte>(w >> 16), static_cast<std::byte>(w >> 24)};
        STARPACK sp{};
        std::array<std::byte, 6> rgb;
        rgb.fill(std::byte{0xa5});
        CHECK(FDecodeStarpack(golden, sp) && sp.dx == w % 1024 && sp.y == (w / 1024) % 4096 && sp.id == w / 4194304);
        CHECK(FEncodeStarpack(sp, std::span(rgb).subspan(1, 4)) && !std::memcmp(rgb.data() + 1, golden.data(), 4));
        CHECK(rgb.front() == std::byte{0xa5} && rgb.back() == std::byte{0xa5});
        if (i == 32768) for (std::size_t cb = 0; cb < 4; cb++) {
            sp.dx = 123; sp.y = 456; sp.id = 789;
            CHECK(!FDecodeStarpack(std::span(golden).first(cb), sp) && sp.dx == 123 && sp.y == 456 && sp.id == 789);
            rgb.fill(std::byte{0xa5});
            CHECK(!FEncodeStarpack(sp, std::span(rgb).subspan(1, cb)));
            for (auto ch : rgb) CHECK(ch == std::byte{0xa5});
        }
    }

    for (std::uint32_t w = 0; w < 65536; w++) {
        const std::uint32_t flags = 65535 - w;
        const std::array<std::byte, 16> golden = {std::byte{0x4a}, std::byte{0x33}, std::byte{0x4a}, std::byte{0x33},
            std::byte{1}, std::byte{0}, std::byte{0}, std::byte{0x80}, static_cast<std::byte>(w % 256), static_cast<std::byte>(w / 256),
            std::byte{0xfe}, std::byte{0xff}, static_cast<std::byte>(w % 256), static_cast<std::byte>(w / 256),
            static_cast<std::byte>(flags % 256), static_cast<std::byte>(flags / 256)};
        const std::int16_t player = static_cast<std::int16_t>((w % 32) < 16 ? std::int32_t(w % 32) : std::int32_t(w % 32) - 32);
        const std::int16_t salt = static_cast<std::int16_t>((w / 32) < 1024 ? std::int32_t(w / 32) : std::int32_t(w / 32) - 2048);
        RTBOF bof{};
        std::array<std::byte, 18> rgb;
        rgb.fill(std::byte{0xa5});
        CHECK(FDecodeBof(golden, bof) && std::memcmp(bof.rgid, "J3J3", 4) == 0 && bof.lidGame == -2147483647 && bof.turn == 65534);
        CHECK(bof.wVersion() == w && bof.verInc == w % 32 && bof.verMinor == (w / 32) % 128 && bof.verMajor == w / 4096);
        CHECK(bof.iPlayer == player && bof.lSaltTime == salt && bof.dt == flags % 256 && bof.wGen == flags / 8192);
        CHECK(bof.fDone == ((flags >> 8) & 1) && bof.fInUse == ((flags >> 9) & 1) && bof.fMulti == ((flags >> 10) & 1));
        CHECK(bof.fGameOverMan == ((flags >> 11) & 1) && bof.fCrippled == ((flags >> 12) & 1));
        CHECK(FEncodeBof(bof, std::span(rgb).subspan(1, 16)) && std::memcmp(rgb.data() + 1, golden.data(), 16) == 0);
        CHECK(rgb[0] == std::byte{0xa5} && rgb[17] == std::byte{0xa5});
        auto view = BfvFromPrefix(reinterpret_cast<std::uint8_t *>(rgb.data() + 1), 16);
        CHECK(view.lidGame() == bof.lidGame && view.wVersion() == w && view.turn() == 65534 && view.iPlayer() == player && view.lSaltTime() == salt);
        CHECK(view.verInc() == bof.verInc && view.verMinor() == bof.verMinor && view.verMajor() == bof.verMajor);
        CHECK(view.dt() == bof.dt && view.fDone() == bof.fDone && view.fInUse() == bof.fInUse && view.fMulti() == bof.fMulti);
        CHECK(view.fGameOverMan() == bof.fGameOverMan && view.fCrippled() == bof.fCrippled && view.wGen() == bof.wGen);
        if (w == 32768) for (std::size_t cb = 0; cb < 16; cb++) {
            bof.lidGame = 999; bof.turn = 123; bof.iPlayer = -7; bof.lSaltTime = -123;
            CHECK(!FDecodeBof(std::span(golden).first(cb), bof) && bof.lidGame == 999 && bof.turn == 123 && bof.iPlayer == -7 && bof.lSaltTime == -123);
            rgb.fill(std::byte{0xa5});
            CHECK(!FEncodeBof(bof, std::span(rgb).subspan(1, cb)));
            for (auto ch : rgb) CHECK(ch == std::byte{0xa5});
        }
    }

    for (std::uint32_t w = 0; w < 65536; w++) {
        const std::array<std::byte, 2> golden = {static_cast<std::byte>(w % 256), static_cast<std::byte>(w / 256)};
        HDR hdr{};
        std::array<std::byte, 4> rgb;
        rgb.fill(std::byte{0xa5});
        CHECK(FDecodeHdr(golden, hdr) && hdr.cb == w % 1024 && hdr.rt == w / 1024);
        CHECK(FEncodeHdr(hdr, std::span(rgb).subspan(1, 2)));
        CHECK(rgb[0] == std::byte{0xa5} && rgb[3] == std::byte{0xa5} && std::memcmp(rgb.data() + 1, golden.data(), 2) == 0);
        CHECK(HdvFromPrefix(reinterpret_cast<std::uint8_t *>(rgb.data() + 1)).cb() == w % 1024);
        CHECK(HdvFromPrefix(reinterpret_cast<std::uint8_t *>(rgb.data() + 1)).rt() == w / 1024);
        for (std::size_t cb = 0; cb < 2; cb++) {
            hdr.cb = 999; hdr.rt = 61;
            CHECK(!FDecodeHdr(std::span(golden).first(cb), hdr) && hdr.cb == 999 && hdr.rt == 61);
            rgb.fill(std::byte{0xa5});
            CHECK(!FEncodeHdr(hdr, std::span(rgb).subspan(1, cb)));
            for (auto ch : rgb) CHECK(ch == std::byte{0xa5});
        }
    }

    struct HISTVECTOR { std::int16_t cPlanet, cPlanetExtra; std::array<std::byte, 4> rgb; };
    const HISTVECTOR rgcase[] = {
        {0, 0, {std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}}},
        {1000, 16, {std::byte{0xe8}, std::byte{3}, std::byte{0x10}, std::byte{0}}},
        {1, -1, {std::byte{1}, std::byte{0}, std::byte{0xff}, std::byte{0xff}}},
        {-32768, 32767, {std::byte{0}, std::byte{0x80}, std::byte{0xff}, std::byte{0x7f}}},
    };
    for (const auto &v : rgcase) {
        RTHISTHDR rthh = {123, 456};
        std::array<std::byte, 6> rgb;
        rgb.fill(std::byte{0xa5});
        CHECK(FDecodeRtHistHdr(v.rgb, rthh) && rthh.cPlanet == v.cPlanet && rthh.cPlanetExtra == v.cPlanetExtra);
        CHECK(FEncodeRtHistHdr(rthh, std::span(rgb).subspan(1, 4)));
        CHECK(std::memcmp(rgb.data() + 1, v.rgb.data(), 4) == 0 && rgb[0] == std::byte{0xa5} && rgb[5] == std::byte{0xa5});
        rthh = {};
        CHECK(FDecodeRtHistHdr(std::span(rgb).subspan(1), rthh) && rthh.cPlanet == v.cPlanet && rthh.cPlanetExtra == v.cPlanetExtra);
        for (std::size_t cb = 0; cb < 4; cb++) {
            rthh = {123, 456};
            CHECK(!FDecodeRtHistHdr(std::span(v.rgb).first(cb), rthh) && rthh.cPlanet == 123 && rthh.cPlanetExtra == 456);
            rgb.fill(std::byte{0xa5});
            CHECK(!FEncodeRtHistHdr(rthh, std::span(rgb).subspan(1, cb)));
            for (auto ch : rgb) CHECK(ch == std::byte{0xa5});
        }
    }
    for (std::uint32_t w = 0; w < 65536; w++) {
        std::uint32_t v = 65535 - w;
        const std::array<std::byte, 4> rgb = {static_cast<std::byte>(w % 256), static_cast<std::byte>(w / 256),
                                           static_cast<std::byte>(v % 256), static_cast<std::byte>(v / 256)};
        const std::int32_t c = w < 32768 ? static_cast<std::int32_t>(w) : static_cast<std::int32_t>(w) - 65536;
        const std::int32_t cExtra = v < 32768 ? static_cast<std::int32_t>(v) : static_cast<std::int32_t>(v) - 65536;
        RTHISTHDR rthh;
        std::array<std::byte, 4> output;
        CHECK(FDecodeRtHistHdr(rgb, rthh) && rthh.cPlanet == c && rthh.cPlanetExtra == cExtra);
        CHECK(FEncodeRtHistHdr(rthh, output) && output == rgb);
    }
    {
        std::array<std::byte, 8> rgb;
        rgb.fill(std::byte{0xa5});
        NATIVEBYTEWRITER output(rgb);
        CHECK(output.FWriteI32Le(std::numeric_limits<std::int32_t>::min()) && output.FWriteU16Le(0x1234));
        CHECK(!output.FWriteU32Le(0) && output.CbWritten() == 6 && rgb[6] == std::byte{0xa5} && rgb[7] == std::byte{0xa5});
        const std::array<std::byte, 6> expected = {std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0x80}, std::byte{0x34}, std::byte{0x12}};
        CHECK(std::memcmp(rgb.data(), expected.data(), expected.size()) == 0);
        NATIVEBYTEREADER input(std::span<const std::byte>(rgb).first(6));
        std::int32_t dw = 0;
        std::uint16_t w = 0;
        std::uint8_t ch = 7;
        CHECK(input.FReadI32Le(dw) && dw == std::numeric_limits<std::int32_t>::min());
        CHECK(input.FReadU16Le(w) && w == 0x1234 && input.CbRead() == 6);
        CHECK(!input.FReadU8(ch) && ch == 7 && input.CbRead() == 6);
        CHECK(!input.FSkip(std::numeric_limits<std::size_t>::max()) && input.CbRemaining() == 0);
    }
    {
        std::array<std::byte, 4> rgb = {std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
        const auto expected = rgb;
        NATIVEBYTEREADER input(std::span<const std::byte>(rgb).first(1));
        std::uint16_t w = 777;
        CHECK(!input.FReadU16Le(w) && w == 777 && input.CbRead() == 0);
        CHECK(!input.FReadBytes(rgb) && rgb == expected && input.CbRead() == 0);
        NATIVEBYTEWRITER output{std::span(rgb).first(1)};
        CHECK(!output.FWriteU16Le(0) && rgb == expected && output.CbWritten() == 0);
        CHECK(!output.FWriteBytes(std::as_bytes(std::span(rgb))) && rgb == expected && output.CbWritten() == 0);
        CHECK(output.FWriteU8(0xff) && rgb[0] == std::byte{0xff});
        NATIVEBYTEREADER empty(std::span<const std::byte>{});
        NATIVEBYTEWRITER emptyOutput(std::span<std::byte>{});
        CHECK(empty.FReadBytes({}) && emptyOutput.FWriteBytes({}) && empty.FSkip(0));
    }
    {
        const std::array<std::byte, 4> rgb = {std::byte{0x78}, std::byte{0x56}, std::byte{0x34}, std::byte{0x12}};
        std::uint32_t dw;
        std::memcpy(&dw, rgb.data(), 4);
        NATIVEBYTEREADER input(std::as_bytes(std::span(&dw, 1)));
        CHECK(input.FReadU32Le(dw) && dw == 0x12345678 && input.CbRead() == 4);
        NATIVEBYTEWRITER output(std::as_writable_bytes(std::span(&dw, 1)));
        CHECK(output.FWriteU32Le(dw) && std::memcmp(&dw, rgb.data(), 4) == 0);
    }
    {
        // Independent S01 bit weights; exercise every single bit, all ones,
        // and a reproducible spread across all fields including reserved bits.
        for (std::uint32_t i = 0; i < 65569; i++) {
            std::uint32_t dw = i < 32 ? (std::uint32_t{1} << i) : i == 32 ? 0xffffffff : (i - 33) * 2654435761u;
            const std::array<std::byte, 4> rgb = {static_cast<std::byte>(dw % 256), static_cast<std::byte>((dw / 256) % 256),
                                                static_cast<std::byte>((dw / 65536) % 256), static_cast<std::byte>(dw / 16777216)};
            PROD prod{};
            std::array<std::byte, 4> output;
            CHECK(FDecodeProd(rgb, prod));
            CHECK(prod.cItem == dw % 1024 && prod.iItem == (dw / 1024) % 128 && prod.grobj == (dw / 131072) % 8 &&
                  prod.pct == (dw / 1048576) % 128 && prod.unused == dw / 134217728);
            CHECK(FEncodeProd(prod, output) && output == rgb);
            for (std::size_t cb = 0; cb < 4; cb++) {
                PROD before = prod;
                output.fill(std::byte{0xa5});
                CHECK(!FDecodeProd(std::span(rgb).first(cb), prod));
                CHECK(prod.cItem == before.cItem && prod.iItem == before.iItem && prod.grobj == before.grobj &&
                      prod.pct == before.pct && prod.unused == before.unused);
                CHECK(!FEncodeProd(prod, std::span(output).first(cb)));
                for (auto ch : output) CHECK(ch == std::byte{0xa5});
            }
        }
        const std::array<std::byte, 8> before = {std::byte{0x11}, std::byte{0x22}, std::byte{0x33}, std::byte{0x44},
                                               std::byte{0x55}, std::byte{0x66}, std::byte{0x77}, std::byte{0x88}};
        const std::array<std::byte, 8> incoming = {std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4},
                                                 std::byte{0xee}, std::byte{0xdd}, std::byte{0xcc}, std::byte{0xbb}};
        for (std::size_t cb = 0; cb <= incoming.size(); cb++) {
            std::array<PROD, 2> rows{};
            std::array<std::byte, 8> output, expected = before;
            CHECK(FDecodeProdBytes(before, rows));
            CHECK(FDecodeProdBytes(std::span(incoming).first(cb), rows));
            std::memcpy(expected.data(), incoming.data(), cb);
            CHECK(FEncodeProdQueue(rows, output) && output == expected);
            if (cb != 0) {
                const auto unchanged = output;
                CHECK(!FDecodeProdBytes(incoming, std::span(rows).first(1)));
                CHECK(!FDecodeProdBytes(std::span(incoming).first(5), std::span(rows).first(1)));
                CHECK(FEncodeProdQueue(rows, output) && output == unchanged);
                output.fill(std::byte{0xa5});
                CHECK(!FEncodeProdQueue(rows, std::span(output).first(7)));
                for (auto ch : output) CHECK(ch == std::byte{0xa5});
            }
        }
        RTCHGPRODQ rtcpq{123};
        const std::array<std::byte, 2> rgbId = {std::byte{0xfe}, std::byte{0xff}};
        CHECK(!FDecodeRtChgProdQ(std::span(rgbId).first(1), rtcpq) && rtcpq.id == 123);
        CHECK(FDecodeRtChgProdQ(rgbId, rtcpq) && rtcpq.id == -2);
        CHECK(FDecodeProdBytes({}, {}) && FEncodeProdQueue({}, {}));
    }
    {
        for (std::uint32_t w = 0; w < 65536; w++) {
            const std::array<std::byte, 2> rgb = {static_cast<std::byte>(w % 256), static_cast<std::byte>(w / 256)};
            ITEMACTION ia{0};
            std::array<std::byte, 2> output;
            CHECK(FDecodeItemAction(rgb, ia) && ia.w == w && ia.cQuan() == w % 4096 && ia.iAction() == w / 4096);
            CHECK(FEncodeItemAction(ia, output) && output == rgb);
            ia.w = static_cast<std::uint16_t>(w ^ 0xa55a);
            const auto action = ia.iAction();
            CHECK(ia.SetCQuan(static_cast<std::uint16_t>(w)) == w % 4096 && ia.iAction() == action);
            CHECK(ia.SetIAction(static_cast<std::uint16_t>(w)) == w % 16 && ia.cQuan() == w % 4096);
            CHECK(ia.w == (w % 4096) + (w % 16) * 4096);
            CHECK(IaMake(static_cast<std::uint16_t>(w), static_cast<std::uint16_t>(w)).w == ia.w);
            for (std::size_t cb = 0; cb < 2; cb++) {
                const auto before = ia.w;
                output.fill(std::byte{0xa5});
                CHECK(!FDecodeItemAction(std::span(rgb).first(cb), ia) && ia.w == before);
                CHECK(!FEncodeItemAction(ia, std::span(output).first(cb)) && output[0] == std::byte{0xa5} && output[1] == std::byte{0xa5});
            }
        }
    }
    {
        const std::array<std::byte, 18> golden = {
            std::byte{0xe8}, std::byte{3}, std::byte{0xff}, std::byte{0x7f}, std::byte{0xff}, std::byte{0xff}, std::byte{0xba}, std::byte{0xfd},
            std::byte{0}, std::byte{0}, std::byte{0xff}, std::byte{0xff}, std::byte{0}, std::byte{0x80}, std::byte{0x34}, std::byte{0x12}, std::byte{0xcd}, std::byte{0xab}};
        ORDER ord{};
        std::array<std::byte, 20> output;
        static_assert(sizeof(ORDER) == 18 && alignof(ORDER) == 2 && sizeof(PLORD) == 4 && alignof(PLORD) == 2);
        CHECK(FDecodeOrder(golden, ord));
        CHECK(ord.pt.x == 1000 && ord.pt.y == 32767 && ord.id == -1 && ord.grTask == 10 && ord.iWarp == 11 && ord.grobj == 13 &&
              ord.fValidTask == 1 && ord.fNoAutoTrack == 1 && ord.fUnused == 3);
        CHECK(ord.txp.rgia[0].w == 0 && ord.txp.rgia[1].w == 65535 && ord.txp.rgia[2].w == 32768 &&
              ord.txp.rgia[3].w == 0x1234 && ord.txp.rgia[4].w == 0xabcd);
        output.fill(std::byte{0xa5});
        CHECK(FEncodeOrder(ord, std::span(output).subspan(1, 18)) && std::memcmp(output.data() + 1, golden.data(), 18) == 0);
        CHECK(output[0] == std::byte{0xa5} && output[19] == std::byte{0xa5});
        for (std::size_t cb = 0; cb < 18; cb++) {
            const auto before = ord;
            CHECK(!FDecodeOrder(std::span(golden).first(cb), ord) && std::memcmp(&ord, &before, sizeof(ord)) == 0);
            output.fill(std::byte{0xa5});
            CHECK(!FEncodeOrder(ord, std::span(output).first(cb)));
            for (auto ch : output) CHECK(ch == std::byte{0xa5});
            if (cb < 8) {
                CHECK(!FDecodeOrderHeader(std::span(golden).first(cb), ord) && std::memcmp(&ord, &before, sizeof(ord)) == 0);
                CHECK(!FEncodeOrderHeader(ord, std::span(output).first(cb)));
                for (auto ch : output) CHECK(ch == std::byte{0xa5});
            }
        }
        for (std::uint32_t w = 0; w < 65536; w++) {
            std::array<std::byte, 8> header{};
            header[6] = static_cast<std::byte>(w % 256);
            header[7] = static_cast<std::byte>(w / 256);
            CHECK(FDecodeOrderHeader(header, ord));
            CHECK(ord.grTask == w % 16 && ord.iWarp == (w / 16) % 16 && ord.grobj == (w / 256) % 16 &&
                  ord.fValidTask == (w / 4096) % 2 && ord.fNoAutoTrack == (w / 8192) % 2 && ord.fUnused == w / 16384);
            CHECK(FEncodeOrderHeader(ord, output) && std::memcmp(header.data(), output.data(), 8) == 0);
            // Decoding the eight-byte form must leave the omitted task words.
            CHECK(ord.txp.rgia[4].w == 0xabcd);
        }
        const std::array<std::byte, 18> incoming = {
            std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8},
            std::byte{9}, std::byte{10}, std::byte{11}, std::byte{12}, std::byte{13}, std::byte{14}, std::byte{15}, std::byte{16}, std::byte{17}, std::byte{18}};
        for (std::size_t cb = 0; cb <= 18; cb++) {
            auto expected = golden;
            CHECK(FDecodeOrder(golden, ord));
            CHECK(FDecodeOrderPrefix(std::span(incoming).first(cb), ord));
            std::memcpy(expected.data(), incoming.data(), cb);
            CHECK(FEncodeOrder(ord, output) && std::memcmp(output.data(), expected.data(), 18) == 0);
        }
        const auto before = ord;
        CHECK(!FDecodeOrderPrefix(output, ord) && std::memcmp(&ord, &before, sizeof(ord)) == 0);
        ord.tlm().cTime = 0xabcd;
        ord.tlm().cTimeOld = 0x1234;
        CHECK(ord.txp.rgia[0].cQuan() == 0xbcd && ord.txp.rgia[0].iAction() == 10 && ord.tsell().iPlrX == 0xabcd);
        const ORDER &readOnly = ord;
        ORDER other{};
        other.txp.rgia[2].w = 0xf00d;
        other.tptl() = readOnly.tptl();
        CHECK(other.tptl().iWarp == 0xabcd && other.tptl().iDist == 0x1234 && other.txp.rgia[2].w == 0xf00d);
        other.tptl() = other.tptl();
        CHECK(other.tlm().cTime == 0xabcd && other.tlm().cTimeOld == 0x1234);
        RTWAYPT rtwp{-2, 32767, ord}, decoded{};
        std::array<std::byte, 22> waypoint;
        CHECK(FEncodeRtWaypt(rtwp, waypoint));
        CHECK(waypoint[0] == std::byte{0xfe} && waypoint[1] == std::byte{0xff} && waypoint[2] == std::byte{0xff} && waypoint[3] == std::byte{0x7f});
        CHECK(FDecodeRtWayptPrefix(waypoint, decoded) && decoded.id == -2 && decoded.iWaypt == 32767);
        CHECK(FDecodeOrder(std::span(waypoint).subspan(4), decoded.order) && decoded.order.tlm().cTime == 0xabcd);
    }

    {
        // S01 HS is u16 category + u8 index + u8 count, even at odd wire offsets.
        for (std::uint32_t w = 0; w < 65536; w++) {
            const std::array<std::byte, 4> golden = {static_cast<std::byte>(w % 256), static_cast<std::byte>(w / 256),
                                                    static_cast<std::byte>((w * 17) % 256), static_cast<std::byte>((w / 256 + 13) % 256)};
            HS hs{};
            std::array<std::byte, 6> output;
            CHECK(FDecodeHs(golden, hs) && hs.grhst == w && hs.iItem == (w * 17) % 256 && hs.cItem == (w / 256 + 13) % 256);
            output.fill(std::byte{0xa5});
            CHECK(FEncodeHs(hs, std::span(output).subspan(1, 4)) && std::memcmp(output.data() + 1, golden.data(), 4) == 0);
            CHECK(output.front() == std::byte{0xa5} && output.back() == std::byte{0xa5});
            for (std::size_t cb = 0; cb < 4; cb++) {
                HS unchanged = hs;
                CHECK(!FDecodeHs(std::span(golden).first(cb), hs) && std::memcmp(&hs, &unchanged, sizeof(hs)) == 0);
                output.fill(std::byte{0xa5});
                CHECK(!FEncodeHs(hs, std::span(output).first(cb)));
                for (auto b : output) CHECK(b == std::byte{0xa5});
            }
        }
        const std::array<std::byte, 26> full = {std::byte{7}, std::byte{0xfd}, std::byte{36}, std::byte{255}, std::byte{0xcd}, std::byte{0xab}, std::byte{2},
            std::byte{0x34}, std::byte{0x12}, std::byte{0x78}, std::byte{0x56}, std::byte{0x34}, std::byte{0x12}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
            std::byte{0}, std::byte{0x80}, std::byte{0xff}, std::byte{0}, std::byte{0xff}, std::byte{0xff}, std::byte{0}, std::byte{0xff}, std::byte{0xa5}};
        for (std::uint32_t w = 0; w < 65536; w++) {
            auto input = full;
            input[0] = static_cast<std::byte>(w % 256); input[1] = static_cast<std::byte>(w / 256);
            SHDEF shdef{};
            std::size_t ibName = 999;
            std::array<std::byte, 28> output;
            CHECK(FDecodeShDefFixed(input, shdef, ibName));
            CHECK(shdef.det == w % 256 && shdef.fInclude == (w / 256) % 2 && shdef.fFree == (w / 512) % 2 && shdef.ishdef == (w / 1024) % 32 && shdef.fGift == w / 32768);
            CHECK(shdef.wFlags() == w && shdef.hul.ihuldef == 36 && shdef.hul.ibmp == 255 && shdef.hul.chs == 2);
            if (shdef.det == 7) {
                CHECK(ibName == 25 && shdef.hul.dp == 0xabcd && shdef.turn == 0x1234 && shdef.cBuilt == 0x12345678 && shdef.cExist == 0xffffffff);
                CHECK(shdef.hul.rghs[0].grhst == 0x8000 && shdef.hul.rghs[0].iItem == 255 && shdef.hul.rghs[1].cItem == 255);
            } else CHECK(ibName == 6 && shdef.hul.wtEmpty == 0xabcd && shdef.turn == 0 && shdef.hul.rghs[0].grhst == 0);
            output.fill(std::byte{0xa5});
            CHECK(FEncodeShDefFixed(shdef, std::span(output).subspan(1), ibName));
            CHECK(std::memcmp(output.data() + 1, input.data(), ibName) == 0 && output[0] == std::byte{0xa5} && output[ibName + 1] == std::byte{0xa5});
            RTCHGSHDEF rtchg{};
            CHECK(FDecodeRtChgShDefPrefix(input, rtchg) && rtchg.mdChg == w % 16 && rtchg.iPlr == (w / 16) % 16 && rtchg.ishdef == (w / 256) % 32 && rtchg.junk == w / 8192);
            CHECK(FEncodeRtChgShDefPrefix(rtchg, output) && output[0] == input[0] && output[1] == input[1]);
        }
        {
            SHDEF shdef{};
            std::size_t cb;
            CHECK(FDecodeShDefFixed(full, shdef, cb));
            auto storage = std::as_writable_bytes(std::span(&shdef, 1));
            CHECK(FEncodeShDefFixed(shdef, storage.subspan(1), cb) && cb == 25);
            CHECK(std::memcmp(storage.data() + 1, full.data(), cb) == 0);
            RTCHGSHDEF rtchg{};
            rtchg.mdChg = 15; rtchg.iPlr = 14; rtchg.ishdef = 31; rtchg.junk = 7;
            const auto before = rtchg;
            std::array<std::byte, 2> output;
            for (std::size_t cut = 0; cut < 2; cut++) {
                CHECK(!FDecodeRtChgShDefPrefix(std::span(full).first(cut), rtchg) && std::memcmp(&rtchg, &before, sizeof(rtchg)) == 0);
                output.fill(std::byte{0xa5});
                CHECK(!FEncodeRtChgShDefPrefix(rtchg, std::span(output).first(cut)) && output[0] == std::byte{0xa5} && output[1] == std::byte{0xa5});
            }
        }
        for (unsigned c = 0; c < 256; c++) {
            std::array<std::byte, 83> rgb;
            for (std::size_t i = 0; i < rgb.size(); i++) rgb[i] = static_cast<std::byte>((i * 19 + 3) % 256);
            rgb[0] = std::byte{7}; rgb[6] = static_cast<std::byte>(c);
            SHDEF shdef{};
            std::size_t cb = 17 + c * 4, ibName = 999;
            const auto before = shdef;
            if (c > 16) {
                CHECK(!FDecodeShDefFixed(rgb, shdef, ibName) && ibName == 999 && std::memcmp(&shdef, &before, sizeof(shdef)) == 0);
            } else {
                for (std::size_t cut = 0; cut < cb; cut++) {
                    CHECK(!FDecodeShDefFixed(std::span(rgb).first(cut), shdef, ibName) && ibName == 999 && std::memcmp(&shdef, &before, sizeof(shdef)) == 0);
                }
                CHECK(FDecodeShDefFixed(std::span(rgb).first(cb), shdef, ibName) && ibName == cb);
                std::array<std::byte, 83> output;
                for (std::size_t cut = 0; cut < cb; cut++) {
                    output.fill(std::byte{0xa5}); ibName = 999;
                    CHECK(!FEncodeShDefFixed(shdef, std::span(output).subspan(1, cut), ibName) && ibName == 999);
                    for (auto b : output) CHECK(b == std::byte{0xa5});
                }
                CHECK(FEncodeShDefFixed(shdef, std::span(output).subspan(1), ibName) && ibName == cb && std::memcmp(output.data() + 1, rgb.data(), cb) == 0);
                CHECK(output[0] == std::byte{0xa5} && output[cb + 1] == std::byte{0xa5});
            }
            // The byte at six is a name marker in the partial form, for all values.
            rgb[0] = std::byte{1};
            CHECK(FDecodeShDefFixed(std::span(rgb).first(7), shdef, ibName) && ibName == 6 && shdef.hul.chs == c);
        }
    }

    {
        static_assert(cbWireFleetIds == 0 && offsetof(RTFLEETIDS, rgid) == 0);
        for (std::uint32_t w = 0; w < 65536; w++) {
            DV dv{static_cast<std::uint16_t>(w)};
            CHECK(dv.pctSh() == w % 128 && dv.pctDp() == w / 128);
            dv.SetPctSh(static_cast<std::uint16_t>(65535 - w));
            CHECK(dv.dp == (w / 128) * 128 + (65535 - w) % 128);
            dv.SetPctDp(static_cast<std::uint16_t>(w));
            CHECK(dv.dp == (w % 512) * 128 + (65535 - w) % 128);
            const auto signedWord = static_cast<std::int16_t>(w < 32768 ? static_cast<std::int32_t>(w) : static_cast<std::int32_t>(w) - 65536);
            FLEET fl{};
            fl.id = signedWord;
            CHECK(fl.ifl() == w % 512 && fl.iplr() == (w / 512) % 16 && fl.junk() == w / 8192);
            fl.SetIfl(65535);
            CHECK(static_cast<std::uint16_t>(fl.id) == (w / 512) * 512 + 511);
            fl.SetIplr(static_cast<std::uint16_t>(w));
            CHECK(static_cast<std::uint16_t>(fl.id) == (w / 8192) * 8192 + (w % 16) * 512 + 511);
            fl.SetDMoveLeft(signedWord); fl.SetDMoveUsed(static_cast<std::int16_t>(-1));
            CHECK(fl.dMoveLeft() == signedWord && fl.dMoveUsed() == -1 && static_cast<std::uint32_t>(fl.lPower) == 0xffff0000u + w);
            fl.dirLong = static_cast<std::int32_t>(w * 2654435761u);
            const std::uint32_t dw = static_cast<std::uint32_t>(fl.dirLong);
            CHECK(fl.dirFltX() == dw % 256 && fl.dirFltY() == (dw / 256) % 256 && fl.iwarpFlt() == (dw / 65536) % 16);
            CHECK(fl.fdirValid() == (dw / 1048576) % 2 && fl.fCompChg() == (dw / 2097152) % 2 && fl.fTargeted() == (dw / 4194304) % 2 && fl.fSkipped() == (dw / 8388608) % 2 && fl.fUnused() == dw / 16777216);
            fl.SetFCompChg(0);
            CHECK(static_cast<std::uint32_t>(fl.dirLong) == (dw & 0xffdfffff));
            fl.SetDirFltX(65535);
            CHECK(fl.dirFltX() == 255 && (static_cast<std::uint32_t>(fl.dirLong) & 0xffffff00) == (dw & 0xffdfff00));
            std::array<std::byte, 14> header = {std::byte{0xfe}, std::byte{0xff}, std::byte{0xff}, std::byte{0x7f}, static_cast<std::byte>(w % 256), static_cast<std::byte>(w / 256),
                std::byte{0xff}, std::byte{0xff}, std::byte{0}, std::byte{0x80}, std::byte{0xe8}, std::byte{3}, std::byte{1}, std::byte{0x80}};
            std::uint16_t mask = 0;
            std::array<std::byte, 16> output;
            CHECK(FDecodeRtFleetHeader(header, fl, mask) && mask == 0x8001 && fl.id == -2 && fl.iPlayer == 32767 && fl.idPlanet == -1 && fl.pt.x == -32768 && fl.pt.y == 1000);
            CHECK(fl.det == w % 256 && fl.fInclude == (w / 256) % 2 && fl.fRepOrders == (w / 512) % 2 && fl.fDead == (w / 1024) % 2 && fl.fDone == (w / 2048) % 2);
            CHECK(fl.fBombed == (w / 4096) % 2 && fl.fHereAllTurn == (w / 8192) % 2 && fl.fNoHeal == (w / 16384) % 2 && fl.fMark == w / 32768);
            output.fill(std::byte{0xa5});
            CHECK(FEncodeRtFleetHeader(fl, fl.fDone, mask, std::span(output).subspan(1, 14)) && std::memcmp(output.data() + 1, header.data(), 14) == 0);
            CHECK(output.front() == std::byte{0xa5} && output.back() == std::byte{0xa5});
            const std::array<std::byte, 3> idBytes = {static_cast<std::byte>(w % 256), static_cast<std::byte>(w / 256), std::byte{0xa5}};
            std::int16_t id = 123;
            CHECK(FReadFleetId({idBytes}, 0, id) && id == signedWord);
            CHECK(!FReadFleetId({idBytes}, 1, id) && id == signedWord);
            CHECK(!FReadFleetId({idBytes}, std::numeric_limits<std::size_t>::max(), id) && id == signedWord);
        }
        for (auto wt : {std::numeric_limits<std::int32_t>::min(), -1, 0, 0x12345678, std::numeric_limits<std::int32_t>::max()}) {
            FLEET fl{};
            fl.rgdv[15].dp = 0xabcd; fl.SetWtFleet(wt);
            CHECK(fl.wtFleet() == wt && fl.rgdv[0].dp == static_cast<std::uint16_t>(wt) && fl.rgdv[1].dp == static_cast<std::uint32_t>(wt) / 65536 && fl.rgdv[15].dp == 0xabcd);
        }
        const std::array<std::byte, 37> golden = {std::byte{0x34}, std::byte{0x12}, std::byte{0xff}, std::byte{0xff}, std::byte{7}, std::byte{0xfd}, std::byte{0xff}, std::byte{0xff}, std::byte{0xe8}, std::byte{3}, std::byte{0}, std::byte{0x80}, std::byte{1}, std::byte{0x80},
            std::byte{0xff}, std::byte{1}, std::byte{0x39}, std::byte{3}, std::byte{0xff}, std::byte{0}, std::byte{1}, std::byte{0}, std::byte{0}, std::byte{1}, std::byte{0}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0x7f},
            std::byte{1}, std::byte{0x80}, std::byte{0xff}, std::byte{0xff}, std::byte{0x34}, std::byte{0x12}, std::byte{0xff}, std::byte{0xfe}};
        FLEET fl{};
        fl.id = 0x1234; fl.iPlayer = -1; fl.det = 7; fl.fInclude = 1; fl.fDead = 1; fl.fBombed = 1; fl.fHereAllTurn = 1; fl.fNoHeal = 1; fl.fMark = 1;
        fl.idPlanet = -1; fl.pt.x = 1000; fl.pt.y = -32768; fl.rgcsh[0] = 255; fl.rgcsh[15] = 1;
        fl.rgwtMin[0] = 255; fl.rgwtMin[1] = 256; fl.rgwtMin[2] = 65536; fl.rgwtMin[3] = -1; fl.rgwtMin[4] = 0x7fffffff;
        fl.rgdv[0].dp = 0xffff; fl.rgdv[15].dp = 0x1234; fl.iplan = 255; fl.cord = 254;
        std::array<std::byte, 134> output;
        std::size_t cb = 999;
        output.fill(std::byte{0xa5});
        CHECK(FEncodeRtFleet(fl, 0, output, cb) && cb == golden.size() && std::memcmp(output.data(), golden.data(), cb) == 0 && output[cb] == std::byte{0xa5} && !fl.fDone);
        for (std::size_t cut = 0; cut < golden.size(); cut++) {
            output.fill(std::byte{0xa5}); cb = 999;
            CHECK(!FEncodeRtFleet(fl, 0, std::span(output).first(cut), cb) && cb == 999);
            for (auto b : output) CHECK(b == std::byte{0xa5});
        }
        FLEET decoded{};
        std::int16_t iDamage = 999;
        CHECK(FDecodeRtFleet(golden, decoded, true, iDamage) && iDamage == 16 && decoded.rgcsh[0] == 255 && decoded.rgcsh[15] == 1);
        CHECK(decoded.rgwtMin[2] == 65536 && decoded.rgwtMin[3] == 0 && decoded.rgwtMin[4] == 0x7fffffff && decoded.iplan == 255 && decoded.cord == 254);
        CHECK(decoded.rgdv[0].pctDp() == 499 && decoded.rgdv[0].pctSh() == 127 && decoded.rgdv[15].dp == 0x1234);
        decoded = {}; iDamage = 999;
        CHECK(!FDecodeRtFleet(golden, decoded, false, iDamage) && decoded.rgcsh[0] == 255 && decoded.rgwtMin[4] == 0x7fffffff && decoded.rgdv[0].dp == 0 && decoded.cord == 0 && iDamage == 999);
        for (std::uint32_t mask = 0; mask < 65536; mask++) for (unsigned phase = 0; phase < 2; phase++) {
            FLEET source{};
            source.det = 1; source.dirLong = 0x12345678;
            unsigned count = 0;
            for (unsigned i = 0; i < 16; i++) {
                source.rgcsh[i] = mask & (1u << i) ? static_cast<std::int16_t>((phase ? 256 : 1) + i) : -1;
                if (mask & (1u << i)) count++;
            }
            CHECK(FEncodeRtFleet(source, -1, output, cb) && cb == 22 + count * (phase ? 2 : 1));
            CHECK(std::to_integer<unsigned>(output[12]) == mask % 256 && std::to_integer<unsigned>(output[13]) == mask / 256);
            std::size_t ib = 14;
            for (unsigned i = 0; i < 16; i++) if (mask & (1u << i)) {
                CHECK(std::to_integer<unsigned>(output[ib++]) == static_cast<unsigned>(source.rgcsh[i]) % 256);
                if (phase) CHECK(std::to_integer<unsigned>(output[ib++]) == 1);
            }
            const std::array<std::byte, 8> suffix = {std::byte{0x78}, std::byte{0x56}, std::byte{0x34}, std::byte{0x12}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff}};
            CHECK(std::memcmp(output.data() + ib, suffix.data(), 8) == 0);
            decoded = {}; iDamage = 999;
            CHECK(FDecodeRtFleet(std::span(output).first(cb), decoded, false, iDamage) && decoded.wtFleet() == -1 && decoded.dirLong == 0x12345678 && iDamage == 999);
            CHECK(decoded.fDone == (phase == 0 || mask == 0) && decoded.fDead == (mask == 0));
            for (unsigned i = 0; i < 16; i++) CHECK(decoded.rgcsh[i] == (mask & (1u << i) ? source.rgcsh[i] : 0));
        }
        // Every cargo width mask, including signed dwords and partial-record
        // population/fuel that the writer omits but the reader still accepts.
        for (unsigned detail : {4u, 7u, 8u, 255u}) for (unsigned widths = 0; widths < 1024; widths++) {
            std::array<std::byte, 134> input{};
            input[4] = static_cast<std::byte>(detail);
            std::size_t ib = 14;
            auto put = [&](std::uint32_t v, unsigned n) { for (unsigned j = 0; j < n; j++) input[ib++] = static_cast<std::byte>((v >> (8 * j)) & 255); };
            put(widths | 0xfc00, 2);
            for (unsigned i = 0; i < 5; i++) {
                unsigned width = (widths >> (2 * i)) & 3;
                if (width) put(width == 1 ? 255 : width == 2 ? 65535 : 0x80000000u + i, width == 3 ? 4 : width);
            }
            if (detail < 7) { put(0x12345678, 4); put(0xffffffff, 4); }
            else { put(0, 2); put(255, 1); put(254, 1); }
            decoded = {}; iDamage = 999;
            CHECK(FDecodeRtFleet(std::span(input).first(ib), decoded, true, iDamage));
            for (unsigned i = 0; i < 5; i++) {
                unsigned width = (widths >> (2 * i)) & 3;
                std::int32_t expected = width == 0 ? 0 : width == 1 ? 255 : width == 2 ? 65535 : std::numeric_limits<std::int32_t>::min() + static_cast<std::int32_t>(i);
                CHECK(decoded.rgwtMin[i] == expected);
            }
            if (detail < 7) CHECK(decoded.wtFleet() == -1 && decoded.dirLong == 0x12345678 && iDamage == 999);
            else CHECK(decoded.iplan == 255 && decoded.cord == 254 && iDamage == 0);
        }
        {
            const std::array<std::uint16_t, 5> ids = {0xffff, 0, 0x8000, 0x7fff, 0};
            const std::array<std::byte, 10> expected = {std::byte{0xff}, std::byte{0xff}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0x80}, std::byte{0xff}, std::byte{0x7f}, std::byte{0}, std::byte{0}};
            CHECK(FEncodeFleetIds(ids, output) && std::memcmp(output.data(), expected.data(), 10) == 0);
            for (std::size_t cut = 0; cut < 14; cut++) {
                FLEET unchanged = fl;
                std::uint16_t mask = 123;
                CHECK(!FDecodeRtFleetHeader(std::span(golden).first(cut), fl, mask) && mask == 123 && std::memcmp(&fl, &unchanged, sizeof(fl)) == 0);
            }
            for (std::uint32_t w = 0; w < 65536; w++) {
                RTXFERF rtxf{};
                rtxf.id1 = static_cast<std::uint16_t>(w); rtxf.id2 = static_cast<std::uint16_t>(65535 - w);
                rtxf.grobj1 = w % 16; rtxf.grobj2 = (w / 16) % 16; rtxf.grbitItems = static_cast<std::uint16_t>(w);
                output.fill(std::byte{0xa5});
                CHECK(FEncodeRtXferFHeader(rtxf, std::span(output).subspan(1)));
                CHECK(std::to_integer<unsigned>(output[1]) == w % 256 && std::to_integer<unsigned>(output[2]) == w / 256 && std::to_integer<unsigned>(output[3]) == (65535 - w) % 256 && std::to_integer<unsigned>(output[4]) == (65535 - w) / 256);
                CHECK(std::to_integer<unsigned>(output[5]) == w % 256 && output[6] == output[1] && output[7] == output[2] && output[8] == std::byte{0xa5});
            }
        }
        // Negative word counts remain nonzero stored i16 values; they are not
        // rejected or silently widened to a positive native count.
        std::array<std::byte, 24> negative{};
        negative[4] = std::byte{1}; negative[12] = std::byte{1}; negative[14] = std::byte{0xff}; negative[15] = std::byte{0xff};
        decoded = {}; iDamage = 999;
        CHECK(FDecodeRtFleet(negative, decoded, false, iDamage) && decoded.rgcsh[0] == -1 && !decoded.fDead);
    }

    {
        const std::array<std::byte, 29> golden = {std::byte{0x34}, std::byte{0x12}, std::byte{0xff}, std::byte{2}, std::byte{31}, std::byte{0x99}, std::byte{0}, std::byte{1}, std::byte{63}, std::byte{254},
            std::byte{100}, std::byte{101}, std::byte{102}, std::byte{103}, std::byte{104}, std::byte{0}, std::byte{0x80}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0x7f}, std::byte{0xce}, std::byte{0xfa},
            std::byte{0xcd}, std::byte{0xab}, std::byte{0x34}, std::byte{0x12}, std::byte{0xa7}, std::byte{0xf5}};
        TOK tok{};
        CHECK(FDecodeTok(golden, tok) && tok.id == 0x1234 && tok.iplr == 255 && tok.grobj == 2 && tok.ishdef == 31 && tok.brc == 0x99);
        CHECK(tok.initBase == 0 && tok.initMin == 1 && tok.initMac == 63 && tok.itokTarget == 254 && tok.pctCloak == 100 && tok.pctJam == 101 && tok.pctBC == 102 && tok.pctCap == 103 && tok.pctBeamDef == 104);
        CHECK(tok.wt == 0x8000 && tok.dpShield == 65535 && tok.csh == 32767 && tok.dv.dp == 0xface);
        CHECK(tok.mdTarget1 == 13 && tok.mdTarget2 == 12 && tok.mdTactic == 11 && tok.mdTarget0 == 10 && tok.dxyLim == 4 && tok.dxyMax == 3 && tok.spd == 2 && tok.cTarget == 1);
        CHECK(tok.wFlags() == 0xf5a7);
        std::array<std::byte, 31> output;
        output.fill(std::byte{0xa5});
        CHECK(FEncodeTok(tok, std::span(output).subspan(1, 29)) && std::memcmp(output.data() + 1, golden.data(), 29) == 0 && output.front() == std::byte{0xa5} && output.back() == std::byte{0xa5});
        for (std::size_t cb = 0; cb < 29; cb++) {
            auto before = tok;
            CHECK(!FDecodeTok(std::span(golden).first(cb), tok) && std::memcmp(&tok, &before, sizeof(tok)) == 0);
            output.fill(std::byte{0xa5});
            CHECK(!FEncodeTok(tok, std::span(output).subspan(1, cb)));
            for (auto b : output) CHECK(b == std::byte{0xa5});
        }
        for (std::uint32_t w = 0; w < 65536; w++) {
            auto input = golden;
            input[23] = static_cast<std::byte>(w % 256); input[24] = static_cast<std::byte>(w / 256);
            input[25] = static_cast<std::byte>((65535 - w) % 256); input[26] = static_cast<std::byte>((65535 - w) / 256);
            input[27] = input[23]; input[28] = input[24];
            CHECK(FDecodeTok(input, tok));
            CHECK(tok.mdTarget1 == w % 16 && tok.mdTarget2 == (w / 16) % 16 && tok.mdTactic == (w / 256) % 16 && tok.mdTarget0 == w / 4096);
            CHECK(tok.dxyLim == (65535 - w) % 16 && tok.dxyMax == ((65535 - w) / 16) % 16 && tok.spd == ((65535 - w) / 256) % 16 && tok.cTarget == (65535 - w) / 4096);
            CHECK(tok.fActive == w % 2 && tok.fDetector == (w / 2) % 2 && tok.fTorp == (w / 4) % 2 && tok.fRegen == (w / 8) % 2 && tok.fMoved == (w / 16) % 2 && tok.dzDis == (w / 32) % 32 && tok.dwt == (w / 1024) % 16 && tok.dMovesLeft == w / 16384);
            CHECK(FEncodeTok(tok, output) && std::memcmp(output.data(), input.data(), 29) == 0);
            std::array<std::byte, 6> event = {std::byte{255}, std::byte{15}, static_cast<std::byte>(w % 256), static_cast<std::byte>(w / 256), static_cast<std::byte>(w % 256), static_cast<std::byte>(w / 256)};
            BTLREC br{};
            std::int32_t c = w < 32768 ? static_cast<std::int32_t>(w) : static_cast<std::int32_t>(w) - 65536;
            CHECK(FDecodeBattleRecord(event, br) && br.itok == 255 && br.brcDest == 15 && br.ctok == c && br.iRound == w % 16 && br.dzDis == (w / 16) % 16 && br.itokAttack == w / 256);
            CHECK(FEncodeBattleRecord(br, output) && std::memcmp(output.data(), event.data(), 6) == 0);
        }
        const std::array<std::byte, 8> killBytes = {std::byte{255}, std::byte{0xa5}, std::byte{255}, std::byte{255}, std::byte{0x34}, std::byte{0x12}, std::byte{255}, std::byte{255}};
        KILL kill{};
        CHECK(FDecodeKill(killBytes, kill) && kill.itok == 255 && kill.grfWeapon == 0xa5 && kill.cshKill == 65535 && kill.dpShield == 0x1234 && kill.dv.dp == 65535);
        CHECK(FEncodeKill(kill, output) && std::memcmp(output.data(), killBytes.data(), 8) == 0);
        const std::array<std::byte, 14> data = {std::byte{0x34}, std::byte{0x12}, std::byte{16}, std::byte{255}, std::byte{0xaa}, std::byte{0xaa}, std::byte{255}, std::byte{0x80}, std::byte{255}, std::byte{255}, std::byte{0}, std::byte{0x80}, std::byte{255}, std::byte{0x7f}};
        BTLDATA bd{};
        CHECK(FDecodeBattleData(data, bd) && bd.id == 0x1234 && bd.cplr == 16 && bd.ctok == 255 && bd.grfPlr == 0xaaaa && bd.cbData == 0x80ff && bd.idPlanet == 65535 && bd.pt.x == -32768 && bd.pt.y == 32767);
        CHECK(FEncodeBattleData(bd, output) && std::memcmp(output.data(), data.data(), 14) == 0);
        for (std::size_t cb = 0; cb < 14; cb++) {
            auto before = bd;
            CHECK(!FDecodeBattleData(std::span(data).first(cb), bd) && std::memcmp(&bd, &before, sizeof(bd)) == 0);
            output.fill(std::byte{0xa5});
            CHECK(!FEncodeBattleData(bd, std::span(output).first(cb)));
            for (auto b : output) CHECK(b == std::byte{0xa5});
        }
    }

    {
        // A view does not eagerly read a complete header/token. These bounded
        // prefixes exercise the fields that the original could inspect alone.
        std::array<std::byte, 2> sentinel = {std::byte{255}, std::byte{255}};
        NATIVEBATTLEDATAVIEW shortData{sentinel, 0};
        CHECK(shortData.id() == 65535);
        std::array<std::byte, 3> tokenPrefix = {std::byte{0x34}, std::byte{0x12}, std::byte{255}};
        NATIVEBATTLETOKENVIEW shortTok{tokenPrefix, 0};
        CHECK(shortTok.id() == 0x1234 && shortTok.iplr() == 255);
        std::array<std::byte, 5> eventPrefix = {std::byte{1}, std::byte{2}, std::byte{255}, std::byte{255}, std::byte{0xa5}};
        NATIVEBATTLEEVENTVIEW shortEvent{eventPrefix, 0};
        CHECK(shortEvent.ctok() == -1 && shortEvent.iRound() == 5 && shortEvent.dzDis() == 10);
        shortEvent.SetIRound(65535);
        CHECK(eventPrefix[4] == std::byte{0xaf});
        shortEvent.SetDzDis(0);
        CHECK(eventPrefix[4] == std::byte{15});
        std::array<std::byte, 12> pointPrefix{};
        NATIVEBATTLEDATAVIEW pointData{pointPrefix, 0};
        pointData.pt().SetX(-32768);
        CHECK(pointData.pt().x() == -32768 && pointPrefix[10] == std::byte{0} && pointPrefix[11] == std::byte{0x80});
        std::array<std::byte, 28> lowFlags{};
        lowFlags[27] = std::byte{0x17};
        NATIVEBATTLETOKENVIEW partialTok{lowFlags, 0};
        CHECK(partialTok.fActive() && partialTok.fDetector() && partialTok.fTorp() && !partialTok.fRegen() && partialTok.fMoved());
        std::array<std::byte, 64> rgb;
        rgb.fill(std::byte{0xa5});
        NATIVEBATTLEEVENTVIEW event{rgb, 16};
        for (std::uint32_t w = 0; w < 65536; w++) {
            event.SetIRound(static_cast<std::uint16_t>(w));
            event.SetDzDis(static_cast<std::uint16_t>(65535 - w));
            event.SetItokAttack(static_cast<std::uint16_t>(w));
            CHECK(std::to_integer<unsigned>(rgb[20]) == w % 16 + ((65535 - w) % 16) * 16 && std::to_integer<unsigned>(rgb[21]) == w % 256);
            auto dv = event.rgkill(0).dv();
            dv.SetDp(static_cast<std::uint16_t>(w));
            CHECK(dv.pctSh() == w % 128 && dv.pctDp() == w / 128);
            dv.SetPctSh(static_cast<std::uint16_t>(65535 - w));
            CHECK(dv.dp() == (w / 128) * 128 + (65535 - w) % 128);
            dv.SetPctDp(static_cast<std::uint16_t>(w));
            CHECK(dv.dp() == (w % 512) * 128 + (65535 - w) % 128);
        }
        CHECK(rgb[15] == std::byte{0xa5} && rgb[22] == std::byte{0xa5} && rgb[30] == std::byte{0xa5});
        event.SetCtok(-1);
        CHECK(event.Next() == reinterpret_cast<std::uint8_t *>(rgb.data()) + 14);
        CHECK(event.rgkill(-1).Lpb() == event.Next());
        auto before = rgb;
        std::array<std::byte, 64> other{};
        NATIVEBATTLEEVENTVIEW rebound{other, 8};
        event = rebound;
        event.SetCtok(42);
        CHECK(rgb == before && other[10] == std::byte{42} && other[11] == std::byte{0});
        CHECK(event.ctok() == 42);
        NATIVEBATTLEDATAVIEW data{rgb, 0};
        data.SetCtok(0);
        // Access remains governed by the physical arena, not a new count gate.
        CHECK(data.rgtok(1).Lpb() == reinterpret_cast<std::uint8_t *>(rgb.data()) + 43);
    }
    {
        const std::array<std::byte, 20> golden = {std::byte{0}, std::byte{0x80}, std::byte{0xff}, std::byte{0x7f},
            std::byte{0xff}, std::byte{0xff}, std::byte{0}, std::byte{0}, std::byte{1}, std::byte{0}, std::byte{0xff}, std::byte{0x7f},
            std::byte{0}, std::byte{0x80}, std::byte{0x34}, std::byte{0x12}, std::byte{0xcd}, std::byte{0xab}, std::byte{0xfe}, std::byte{0xff}};
        AISTARBASE asb{};
        CHECK(FDecodeAiStarbase(golden, asb));
        CHECK(asb.idPlanet == -32768 && asb.cFreighter == 32767 && asb.rgflid[0] == -1 && asb.rgflid[6] == -21555 && asb.rgflid[7] == -2);
        std::array<std::byte, 22> encoded;
        encoded.fill(std::byte{0xa5});
        CHECK(FEncodeAiStarbase(asb, std::span(encoded).subspan(1, 20)));
        CHECK(std::memcmp(encoded.data() + 1, golden.data(), 20) == 0 && encoded.front() == std::byte{0xa5} && encoded.back() == std::byte{0xa5});
        for (std::size_t cb = 0; cb < 20; cb++) {
            AISTARBASE unchanged = asb;
            CHECK(!FDecodeAiStarbase(std::span(golden).first(cb), unchanged));
            CHECK(unchanged.idPlanet == asb.idPlanet && unchanged.cFreighter == asb.cFreighter && unchanged.rgflid[7] == -2);
            encoded.fill(std::byte{0xa5});
            CHECK(!FEncodeAiStarbase(asb, std::span(encoded).subspan(1, cb)));
            for (auto b : encoded) CHECK(b == std::byte{0xa5});
        }
        std::array<std::byte, 1320> rgb;
        rgb.fill(std::byte{0xa5});
        NATIVEAIHISTORYVIEW hist{rgb, 1};
        NATIVEAICYBERVIEW cyber{rgb, 1};
        NATIVEAICYBERTEMPVIEW temp{rgb, 3};
        NATIVEAIHISTSIZEVIEW length{rgb, 1};
        for (std::uint32_t w = 0; w < 65536; w++) {
            const std::int32_t signedWord = w < 32768 ? static_cast<std::int32_t>(w) : static_cast<std::int32_t>(w) - 65536;
            hist.SetCbAiHist(static_cast<std::uint16_t>(w));
            CHECK(length.value() == signedWord && hist.cbAiHist() == w);
            CHECK(cyber.iLstPktDir() == w % 8 && cyber.fBltColony() == (w / 8) % 2 && cyber.fLaunchedPkt() == (w / 16) % 2);
            CHECK(cyber.iPktTarget() == (w / 32) % 4 && cyber.fNeedScanPkt() == (w / 128) % 2 && cyber.unused() == w / 256);
            cyber.SetIPktTarget(static_cast<std::uint16_t>(cyber.iPktTarget() + 0x7ff));
            CHECK(cyber.wInfo() == (w & ~96u) + (((w / 32 + 3) % 4) * 32));
            temp.SetWInfo1(static_cast<std::uint16_t>(w));
            CHECK(temp.fIdleColonizers() == w % 2 && temp.cIdleFreighters() == (w / 2) % 4 && temp.cFreightersDst() == (w / 8) % 4);
            CHECK(temp.fNeedDefenders() == (w / 32) % 2 && temp.fDefended() == (w / 64) % 2 && temp.fUnderAttack() == (w / 128) % 2);
            CHECK(temp.fNeedsMin1() == (w / 256) % 2 && temp.fNeedsMin2() == (w / 512) % 2 && temp.fNeedsMin3() == (w / 1024) % 2 && temp.unused() == w / 2048);
            temp.SetCIdleFreighters(static_cast<std::uint16_t>(w));
            CHECK(temp.wInfo1() == (w & ~6u) + (w % 4) * 2);
        }
        hist.SetCbAiHist(2); hist.SetCStarbase(0);
        hist.rgasb(63).SetValue(asb);
        CHECK(hist.rgasb(63).idPlanet() == -32768 && hist.rgasb(63).rgflid(7) == -2);
        hist.rgasb(64).SetIdPlanet(42); // Physical spare storage is not a new logical admission gate.
        CHECK(hist.rgasb(64).idPlanet() == 42);
        hist.rgasb(0).SetCFreighter(7); hist.rgasb(0).SetNextFlid(-32768);
        CHECK(hist.rgasb(0).cFreighter() == 8 && hist.rgasb(0).rgflid(7) == -32768);
        CHECK(rgb.front() == std::byte{0xa5} && rgb.back() == std::byte{0xa5});
        NATIVEAIMINERALVIEW mineral{rgb, 1};
        mineral.SetValue(std::numeric_limits<std::int32_t>::min());
        CHECK(rgb[1] == std::byte{0} && rgb[4] == std::byte{0x80} && mineral.value() == std::numeric_limits<std::int32_t>::min());
        mineral.SetValue(-1);
        CHECK(rgb[1] == std::byte{0xff} && rgb[4] == std::byte{0xff} && mineral.value() == -1);
        std::array<std::byte, 1> prefix{std::byte{0xff}};
        NATIVEAICYBERVIEW shortCyber{prefix, 0};
        CHECK(shortCyber.iPktTarget() == 3);
        shortCyber.SetFNeedScanPkt(0);
        CHECK(prefix[0] == std::byte{0x7f});
    }
    {
        BTLPLAN plan{};
        std::strcpy(plan.szName, "retained");
        for (std::uint32_t w = 0; w < 65536; w++) {
            const std::uint32_t v = 65535 - w;
            const std::array<std::byte, 4> golden = {static_cast<std::byte>(w % 256), static_cast<std::byte>(w / 256),
                                                    static_cast<std::byte>(v % 256), static_cast<std::byte>(v / 256)};
            CHECK(FDecodeBattlePlanPrefix(golden, plan));
            CHECK(plan.iplr == w % 16 && plan.iplan == (w / 16) % 16 && plan.mdTactic == (w / 256) % 16);
            CHECK(plan.unused1 == (w / 4096) % 4 && plan.fDelete == (w / 16384) % 2 && plan.fDumpCargo == w / 32768);
            CHECK(plan.mdTarget1 == v % 16 && plan.mdTarget2 == (v / 16) % 16 && plan.iplrAttack == (v / 256) % 32 && plan.unused2 == v / 8192);
            CHECK(std::strcmp(plan.szName, "retained") == 0);
            std::array<std::byte, 6> output;
            output.fill(std::byte{0xa5});
            CHECK(FEncodeBattlePlanPrefix(plan, std::span(output).subspan(1, 4)));
            CHECK(std::memcmp(output.data() + 1, golden.data(), 4) == 0 && output.front() == std::byte{0xa5} && output.back() == std::byte{0xa5});
        }
        std::array<std::byte, 4> input{};
        for (std::size_t cb = 0; cb < 4; cb++) {
            plan.iplan = 13; plan.unused2 = 7;
            CHECK(!FDecodeBattlePlanPrefix(std::span(input).first(cb), plan) && plan.iplan == 13 && plan.unused2 == 7);
            input.fill(std::byte{0xa5});
            CHECK(!FEncodeBattlePlanPrefix(plan, std::span(input).first(cb)));
            for (auto b : input) CHECK(b == std::byte{0xa5});
        }
        std::array<std::byte, 1> owner{std::byte{0xd3}};
        NATIVEBATTLEPLANVIEW ownerView{owner, 0};
        CHECK(ownerView.iplr() == 3 && ownerView.iplan() == 13);
        std::array<std::byte, 2> deletion{std::byte{0xd3}, std::byte{0xff}};
        NATIVEBATTLEPLANVIEW deletionView{deletion, 0};
        CHECK(deletionView.fDelete() && deletionView.fDumpCargo() && deletionView.unused1() == 3);
    }
    {
        const std::array<std::byte, 12> golden = {std::byte{0xef}, std::byte{0xbe}, std::byte{0xad}, std::byte{0xde},
            std::byte{0}, std::byte{0x80}, std::byte{0xff}, std::byte{0x7f}, std::byte{0xff}, std::byte{0xff}, std::byte{0xfc}, std::byte{0xff}};
        RTPLRMSG msg{};
        CHECK(FDecodePlrMsgPrefix(golden, msg));
        CHECK(msg.lpmsgplrNext == 0xdeadbeef && msg.iPlrFrom == -32768 && msg.iPlrTo == 32767 && msg.iInRe == -1 && msg.cLen == -4);
        std::array<std::byte, 14> output;
        output.fill(std::byte{0xa5});
        CHECK(FEncodePlrMsgPrefix(msg, std::span(output).subspan(1, 12)));
        CHECK(std::memcmp(output.data() + 1, golden.data(), 12) == 0 && output.front() == std::byte{0xa5} && output.back() == std::byte{0xa5});
        for (std::size_t cb = 0; cb < 12; cb++) {
            RTPLRMSG unchanged = msg;
            CHECK(!FDecodePlrMsgPrefix(std::span(golden).first(cb), unchanged) && unchanged.lpmsgplrNext == msg.lpmsgplrNext && unchanged.cLen == -4);
            output.fill(std::byte{0xa5});
            CHECK(!FEncodePlrMsgPrefix(msg, std::span(output).subspan(1, cb)));
            for (auto b : output) CHECK(b == std::byte{0xa5});
        }
        for (std::uint32_t w = 0; w < 65536; w++) {
            auto rgb = golden;
            rgb[10] = static_cast<std::byte>(w % 256); rgb[11] = static_cast<std::byte>(w / 256);
            CHECK(FDecodePlrMsgPrefix(rgb, msg));
            CHECK(msg.cLen == (w < 32768 ? static_cast<std::int32_t>(w) : static_cast<std::int32_t>(w) - 65536));
            std::array<std::byte, 12> encoded;
            CHECK(FEncodePlrMsgPrefix(msg, encoded) && encoded == rgb);
        }
        std::array<std::byte, 7> partial{};
        partial[4] = std::byte{0xff}; partial[5] = std::byte{0xff};
        NATIVEPLAYERMESSAGEVIEW prefix{partial, 0};
        CHECK(prefix.iPlrFrom() == -1);
    }
    {
        for (std::uint32_t w = 0; w < 65536; w++) {
            const std::uint32_t v = 65535 - w;
            const std::array<std::byte, 5> golden = {static_cast<std::byte>(w % 256), static_cast<std::byte>(w % 256), static_cast<std::byte>(w / 256),
                static_cast<std::byte>(v % 256), static_cast<std::byte>(v / 256)};
            MSGTURN mt{};
            CHECK(FDecodeMsgTurn(golden, mt));
            CHECK(mt.iPlr == w % 16 && mt.cbParams == (w / 16) % 16 && mt.msghdr.iMsg == w % 512 && mt.msghdr.grWord == w / 512);
            CHECK(mt.msghdr.wGoto == (v < 32768 ? static_cast<std::int32_t>(v) : static_cast<std::int32_t>(v) - 65536));
            std::array<std::byte, 7> output;
            output.fill(std::byte{0xa5});
            CHECK(FEncodeMsgTurn(mt, std::span(output).subspan(1, 5)));
            CHECK(std::memcmp(output.data() + 1, golden.data(), 5) == 0 && output.front() == std::byte{0xa5} && output.back() == std::byte{0xa5});
        }
        const std::array<std::byte, 5> golden = {std::byte{0x31}, std::byte{5}, std::byte{4}, std::byte{5}, std::byte{0x80}};
        for (std::size_t cb = 0; cb < 5; cb++) {
            MSGTURN mt{}; mt.iPlr = 13; mt.msghdr.iMsg = 511;
            CHECK(!FDecodeMsgTurn(std::span(golden).first(cb), mt) && mt.iPlr == 13 && mt.msghdr.iMsg == 511);
            std::array<std::byte, 5> output; output.fill(std::byte{0xa5});
            CHECK(!FEncodeMsgTurn(mt, std::span(output).first(cb)));
            for (auto b : output) CHECK(b == std::byte{0xa5});
        }
        std::array<std::byte, 9> backing{};
        std::memcpy(backing.data() + 1, golden.data(), 5);
        NATIVEMESSAGETURNVIEW turn{backing, 1};
        CHECK(turn.iPlr() == 1 && turn.cbParams() == 3 && turn.msghdr().iMsg() == 5 && turn.msghdr().grWord() == 2);
        CHECK(turn.msghdr().wGoto() == static_cast<MsgGoto>(-32763) && turn.Next() == reinterpret_cast<std::uint8_t *>(backing.data()) + 9);
        turn.msghdr().SetIMsg(511);
        CHECK(turn.msghdr().grWord() == 2 && backing[2] == std::byte{0xff} && backing[3] == std::byte{5});
        std::array<std::byte, 1> shortTurn{std::byte{0xf3}};
        NATIVEMESSAGETURNVIEW prefix{shortTurn, 0};
        CHECK(prefix.iPlr() == 3 && prefix.cbParams() == 15);
    }
    {
        const std::array<std::byte, 4> golden = {std::byte{0}, std::byte{0x80}, std::byte{0xff}, std::byte{0xff}};
        RTCHGNAME name{};
        name.rgb[0] = 0x55;
        CHECK(FDecodeChangeNamePrefix(golden, name) && name.id == -32768 && name.grobj == -1 && name.rgb[0] == 0x55);
        std::array<std::byte, 6> output;
        output.fill(std::byte{0xa5});
        CHECK(FEncodeChangeNamePrefix(name, std::span(output).subspan(1, 4)));
        CHECK(std::memcmp(output.data() + 1, golden.data(), 4) == 0 && output.front() == std::byte{0xa5} && output.back() == std::byte{0xa5});
        for (std::size_t cb = 0; cb < 4; cb++) {
            name.id = 42; name.grobj = 43;
            CHECK(!FDecodeChangeNamePrefix(std::span(golden).first(cb), name) && name.id == 42 && name.grobj == 43);
            output.fill(std::byte{0xa5});
            CHECK(!FEncodeChangeNamePrefix(name, std::span(output).subspan(1, cb)));
            for (auto b : output) CHECK(b == std::byte{0xa5});
        }
        for (std::uint32_t w = 0; w < 65536; w++) {
            std::array<std::byte, 2> rgb = {static_cast<std::byte>(w % 256), static_cast<std::byte>(w / 256)};
            RTRESEARCH research{};
            CHECK(FDecodeResearch(rgb, research));
            CHECK(research.pctResearch == (w % 256 < 128 ? static_cast<int>(w % 256) : static_cast<int>(w % 256) - 256));
            CHECK(research.iTechNow() == (w / 256) % 16 && research.iTechNext() == w / 4096);
            std::array<std::byte, 2> encoded;
            CHECK(FEncodeResearch(research, encoded) && encoded == rgb);
            NATIVERESEARCHVIEW view{rgb, 0};
            CHECK(view.pctResearch() == research.pctResearch && view.iTechCur() == research.iTechCur);
            CHECK(view.iTechNow() == research.iTechNow() && view.iTechNext() == research.iTechNext());
        }
        std::array<std::byte, 1> invalid{std::byte{0x80}};
        NATIVERESEARCHVIEW firstByte{invalid, 0};
        CHECK(firstByte.pctResearch() == -128);
        RTLOGHDR header{};
        std::array<std::byte, 17> logGolden{};
        logGolden[1] = std::byte{0x80};
        for (unsigned i = 0; i < 15; i++) logGolden[i + 2] = static_cast<std::byte>(0xa0 + i);
        CHECK(FDecodeLogHdr(logGolden, header) && header.cbLog == -32768 && header.rgbUnused[14] == 0xae);
        std::array<std::byte, 17> encoded;
        CHECK(FEncodeLogHdr(header, encoded) && encoded == logGolden);
        for (std::size_t cb = 0; cb < 17; cb++) {
            header.cbLog = 999; std::memset(header.rgbUnused, 0x55, 15);
            CHECK(!FDecodeLogHdr(std::span(logGolden).first(cb), header) && header.cbLog == 999 && header.rgbUnused[14] == 0x55);
            encoded.fill(std::byte{0xa5});
            CHECK(!FEncodeLogHdr(header, std::span(encoded).first(cb)));
            for (auto b : encoded) CHECK(b == std::byte{0xa5});
        }
    }
    {
        const std::array<std::byte, 6> golden = {std::byte{0x34}, std::byte{0x12}, std::byte{0xcd}, std::byte{0xab}, std::byte{0xf2}, std::byte{0xa5}};
        RTXFER transfer{}; transfer.rgcQuan[0] = -7;
        CHECK(FDecodeCargoXferPrefix(golden, transfer));
        CHECK(transfer.id1 == 0x1234 && transfer.id2 == 0xabcd && transfer.grobj1 == 2 && transfer.grobj2 == 15 && transfer.grbitItems == 0xa5 && transfer.rgcQuan[0] == -7);
        for (std::uint32_t w = 0; w < 65536; w++) {
            const std::uint32_t v = 65535 - w;
            auto rgb = golden;
            rgb[0] = static_cast<std::byte>(w % 256); rgb[1] = static_cast<std::byte>(w / 256);
            rgb[2] = static_cast<std::byte>(v % 256); rgb[3] = static_cast<std::byte>(v / 256);
            CHECK(FDecodeCargoXferPrefix(rgb, transfer) && transfer.id1 == w && transfer.id2 == v);
            std::array<std::byte, 8> output; output.fill(std::byte{0x55});
            CHECK(FEncodeCargoXferPrefix(transfer, std::span(output).subspan(1, 6)));
            CHECK(std::memcmp(output.data() + 1, rgb.data(), 6) == 0 && output.front() == std::byte{0x55} && output.back() == std::byte{0x55});
        }
        for (std::size_t cb = 0; cb < 6; cb++) {
            transfer.id1 = 42; transfer.id2 = 43; transfer.grbitItems = 0x55;
            CHECK(!FDecodeCargoXferPrefix(std::span(golden).first(cb), transfer) && transfer.id1 == 42 && transfer.id2 == 43 && transfer.grbitItems == 0x55);
            std::array<std::byte, 6> output; output.fill(std::byte{0x55});
            CHECK(!FEncodeCargoXferPrefix(transfer, std::span(output).first(cb)));
            for (auto b : output) CHECK(b == std::byte{0x55});
        }
        std::array<std::byte, 5> lookupPrefix{};
        std::memcpy(lookupPrefix.data(), golden.data(), 5);
        NATIVECARGOXFERVIEW prefix{lookupPrefix, 0};
        CHECK(prefix.id1() == 0x1234 && prefix.id2() == 0xabcd && prefix.grobj1() == 2 && prefix.grobj2() == 15);
        std::array<std::byte, 28> rgb; rgb.fill(std::byte{0xa5});
        NATIVECARGOXFERVIEW quantities{rgb, 1};
        for (unsigned w = 0; w < 256; w++) {
            const auto q = static_cast<std::int8_t>(w < 128 ? static_cast<int>(w) : static_cast<int>(w) - 256);
            quantities.SetCQuan8(0, q);
            CHECK(quantities.cQuan8(0) == q && std::to_integer<unsigned>(rgb[7]) == w);
        }
        for (unsigned w = 0; w < 65536; w++) {
            const auto q = static_cast<std::int16_t>(w < 32768 ? static_cast<int>(w) : static_cast<int>(w) - 65536);
            quantities.SetCQuan16(1, q);
            CHECK(quantities.cQuan16(1) == q && std::to_integer<unsigned>(rgb[9]) == w % 256 && std::to_integer<unsigned>(rgb[10]) == w / 256);
        }
        const std::int32_t values[] = {0, 1, -1, 127, -128, 32767, -32768, std::numeric_limits<std::int32_t>::min(), std::numeric_limits<std::int32_t>::max()};
        for (auto q : values) {
            quantities.SetCQuan32(4, q);
            CHECK(quantities.cQuan32(4) == q);
            const auto bits = std::bit_cast<std::uint32_t>(q);
            for (unsigned j = 0; j < 4; j++) CHECK(std::to_integer<unsigned>(rgb[23 + j]) == ((bits >> (8 * j)) & 255));
        }
        CHECK(rgb.front() == std::byte{0xa5} && rgb.back() == std::byte{0xa5});
    }
    std::puts("Universe/starpack/history/production/action/order/design/fleet/battle/AI/message/log wire vectors, signed domains, partial bytes and bounded primitives passed");
    return 0;
}
