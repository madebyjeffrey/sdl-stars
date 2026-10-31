// Bounded views must never read a C-string terminator outside their slice.
#include "encoding.h"
#include <cstdio>
#include <cstring>

#define CHECK(f) do { if (!(f)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
int main() {
    const char rgchText[] = {'A', '\xce', '\xa9', 'B'};
    const std::string_view szText(rgchText, sizeof(rgchText));
    const unsigned char rgbUtf16[] = {0xa9, 0x03};
    const unsigned char rgbCp1252[] = {0xe9, 0x80};
    char sz[16], szSmall[4] = {'x', 'x', 'x', 'x'};
    uint32_t ch = 123;

    CHECK(FUtf8Valid(szText) && !FUtf8Valid(szText.substr(0, 2)));
    CHECK(CbUtf8Char(szText.substr(1, 2), &ch) == 2 && ch == 0x3a9);
    CHECK(CbUtf8Char(std::string_view{}, &ch) == 0 && ch == 0x3a9);
    CHECK(CchUtf8Prefix(szText, 2) == 1 && CchUtf8Prefix(szText, 3) == 3);
    CHECK(CchUtf8Prefix(szText.substr(0, 2), 9) == 1);
    CHECK(CchUtf8Prefix(std::string_view("A\0B", 3), 3) == 1);
    CHECK(CchCopyUtf8(std::span(sz), szText) == 4 && !std::strcmp(sz, "A\xce\xa9" "B"));
    CHECK(CchCopyUtf8(std::span(szSmall).first(3), szText) == -3 && szSmall[0] == 0 && szSmall[3] == 'x');
    CHECK(CchCopyUtf8(std::span(sz), std::string_view("A\0B", 3)) == -1 && sz[0] == 0);
    CHECK(CchCopyUtf8(std::span<char>{}, szText) == -3);
    CHECK(CchCopyUtf8(std::span(sz), std::string_view{}) == 0 && sz[0] == 0);
    std::strcpy(sz, "abcde");
    CHECK(CchCopyUtf8(std::span(sz), std::string_view(sz + 1, 3)) == 3 && !std::strcmp(sz, "bcd"));
    CHECK(CchCp1252ToUtf8(std::span(sz), std::as_bytes(std::span(rgbCp1252))) == 5 && !std::strcmp(sz, "\xc3\xa9\xe2\x82\xac"));
    CHECK(CchUtf16leToUtf8(std::span(sz), std::as_bytes(std::span(rgbUtf16))) == 2 && !std::strcmp(sz, "\xce\xa9"));
    CHECK(CchUtf16leToUtf8(std::span(sz), std::as_bytes(std::span(rgbUtf16)).first(1)) == -1 && sz[0] == 0);
    std::puts("Bounded UTF-8 text, output buffers and byte views passed");
    return 0;
}
