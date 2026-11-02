#include "nativeformat.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <cwchar>
#include <iterator>

#define CHECK(f) do { if (!(f)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #f); return false; } } while (0)

template<class... ARGS> bool FCompare(const char *pszFormat, ARGS... args) {
    // Include a fitting destination as well as every smaller capacity. GCC's
    // fortified CRT diagnoses a reference call that can only ever truncate.
    for (std::size_t cb = 0; cb <= 128; cb++) {
        char rgExpected[136], rgActual[136];
        std::fill(std::begin(rgExpected), std::end(rgExpected), '!');
        std::fill(std::begin(rgActual), std::end(rgActual), '!');
        const int expected = std::snprintf(rgExpected + 3, cb, pszFormat, args..., 0);
        const int actual = NATIVEFORMATTER{}(std::span{rgActual}.subspan(3, cb), pszFormat, args...);
        CHECK(actual == expected);
        CHECK(!std::memcmp(rgActual, rgExpected, sizeof(rgActual)));
    }
    return true;
}

bool FTests() {
    CHECK(FCompare("literal UTF-8 \xc3\xa9 %% end"));
    CHECK(FCompare("%d|%+08i|%#o|%#08x|%X|%u", std::numeric_limits<int>::min(), -31, 077u, 0xfaceu, 0xdefu, std::numeric_limits<unsigned int>::max()));
    CHECK(FCompare("%hhd|%hhu|%hd|%hu", -129, 257u, -32769, 65537u));
    CHECK(FCompare("%ld|%lu|%lld|%llu", std::numeric_limits<long>::min(), std::numeric_limits<unsigned long>::max(), LLONG_MIN, ULLONG_MAX));
    CHECK(FCompare("%jd|%ju|%zd|%zu|%td|%tu", std::intmax_t{-123}, std::uintmax_t{123},
                   std::make_signed_t<std::size_t>{-7}, std::size_t{17}, std::ptrdiff_t{-9}, std::make_unsigned_t<std::ptrdiff_t>{19}));
    CHECK(FCompare("[%*.*s][%*d][%.*d]", -13, 4, "payload", 12, 17, -1, -5));
    CHECK(FCompare("[%6.4d][%-5s][%#x][%hu]", 12, "ab", 42u, 1u));
    CHECK(FCompare("[%.0d][%#.0o][%.0u]", 0, 0u, 0u));
    CHECK(FCompare("%.3s %c %s", "ab\xc3\xa9", 0, "tail"));
    CHECK(FCompare("%.17g|%#.9e|%+012.4f|%a|%A|%F", 0.1, 3.25, -0.0, 0.1, 3.25, std::numeric_limits<double>::infinity()));
    CHECK(FCompare("%.21Lg|%La|%lf", 0.1L, 0.1L, 0.25));
    CHECK(FCompare("%lc|%ls", std::wint_t{'X'}, L"ASCII"));
    auto *lp = reinterpret_cast<char *>(std::uintptr_t{0x1234});
    CHECK(FCompare("%p|%p", static_cast<void *>(lp), static_cast<void *>(nullptr)));
    char sz[32];
    CHECK(NATIVEFORMATTER{}(sz, "%p", lp) == std::snprintf(sz, sizeof(sz), "%p", static_cast<void *>(lp)));
    const char rgch[] = {'a', 'b', 'c', 'x'};
    CHECK(NATIVEFORMATTER{}(sz, "[%5.2s]", std::string_view(rgch, 3)) == 7 && !std::strcmp(sz, "[   ab]"));
    CHECK(NATIVEFORMATTER{}(sz, "%s", std::string("owned")) == 5 && !std::strcmp(sz, "owned"));
    struct { signed int c : 5; unsigned int u : 5; } bits = {-3, 29};
    CHECK(NATIVEFORMATTER{}(sz, "%d %u", bits.c, bits.u) == 5 && !std::strcmp(sz, "-3 29"));
    int count = -1; short shortCount = -1; long long wideCount = -1;
    CHECK(NATIVEFORMATTER{}(sz, "ab%n:%hn:%lln%d", &count, &shortCount, &wideCount, 9) == 5);
    CHECK(count == 2 && shortCount == 3 && wideCount == 4 && !std::strcmp(sz, "ab::9"));
    CHECK(NATIVEFORMATTER{}(std::span{sz}.first(2), "abc%n", &count) == 3 && count == 3 && !std::strcmp(sz, "a"));
    CHECK(NATIVEFORMATTER{}(std::span<char>{}, "abc%n", &count) == 3 && count == 3);
    CHECK(NATIVEFORMATTER{}(sz, "constant", lp) == 8 && !std::strcmp(sz, "constant"));
    errno = 0; CHECK(NATIVEFORMATTER{}(sz, "before %d") == -1 && errno == EINVAL && !std::strcmp(sz, "before "));
    errno = 0; CHECK(NATIVEFORMATTER{}(sz, "%s", 7) == -1 && errno == EINVAL);
    errno = 0; CHECK(NATIVEFORMATTER{}(sz, "%d", "wrong") == -1 && errno == EINVAL);
    errno = 0; CHECK(NATIVEFORMATTER{}(sz, "%n", static_cast<int *>(nullptr)) == -1 && errno == EINVAL);
    errno = 0; CHECK(NATIVEFORMATTER{}(sz, "%") == -1 && errno == EINVAL);
    errno = 0; CHECK(NATIVEFORMATTER{}(sz, nullptr) == -1 && errno == EINVAL && !sz[0]);
    return true;
}
int main() {
    if (!FTests()) return 1;
    std::puts("Typed native formatting matches CRT bytes/counts and protects destination spans");
}
