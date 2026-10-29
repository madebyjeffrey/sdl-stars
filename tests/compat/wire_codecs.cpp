// Exact wire expectations come from S01's little-endian field table, not from
// a native struct dump. Exercise unaligned slices and every signed 16-bit value.
#include "recordcodec.h"
#include <array>
#include <cstdio>
#include <limits>

#define CHECK(f) do { if (!(f)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

int main() {
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
    std::puts("History/production wire vectors, signed domains, partial bytes and bounded primitives passed");
    return 0;
}
