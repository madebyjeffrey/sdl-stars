/* Shared SDK comparisons for the UI's narrow strings and CRT helpers. */
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#ifndef _WIN32
#include <stars_compat.h>
#include <unistd.h>
#else
#include <process.h>
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

int main(void) {
    char sz[1024], szPath[128], szFile[160], szDate[9], szTime[9];
    FILE *fp;
    DWORD idThread = GetCurrentThreadId();
    int i, j, k, n, nExpected;
    int64_t l, lAbs, lDiv, lRounded;
    const int rgn[] = { -100000, -7, -3, -2, -1, 0, 1, 2, 3, 7, 100000 };

    CHECK(idThread != 0 && GetCurrentThreadId() == idThread);
    for (i = 0; i < (int)(sizeof(rgn) / sizeof(rgn[0])); i++)
        for (j = 0; j < (int)(sizeof(rgn) / sizeof(rgn[0])); j++)
            for (k = 0; k < (int)(sizeof(rgn) / sizeof(rgn[0])); k++) {
                l = (int64_t)rgn[i] * rgn[j];
                lAbs = l < 0 ? -l : l;
                lDiv = rgn[k] < 0 ? -(int64_t)rgn[k] : rgn[k];
                lRounded = lDiv ? (lAbs + lDiv / 2) / lDiv : 0;
                if ((l < 0) != (rgn[k] < 0)) lRounded = -lRounded;
                nExpected = !lDiv || lRounded <= INT32_MIN || lRounded > INT32_MAX ? -1 : (int)lRounded;
                n = MulDiv(rgn[i], rgn[j], rgn[k]);
                CHECK(n == nExpected);
            }
    CHECK(MulDiv(INT32_MAX, INT32_MAX, INT32_MAX) == INT32_MAX);
    CHECK(MulDiv(INT32_MIN, 1, 1) == -1);
    CHECK(MulDiv(INT32_MIN, -1, 1) == -1);
    CHECK(lstrlenA(NULL) == 0 && lstrlenA("fleet") == 5);
    CHECK(lstrcpyA(sz, "Fleet") == sz && lstrcatA(sz, " #10") == sz);
    CHECK(!strcmp(sz, "Fleet #10") && lstrcmpiA(sz, "FLEET #10") == 0);
    CHECK(lstrcmpiA("a", "b") < 0 && lstrcmpiA("z", "a") > 0);
    CHECK(wsprintfA(sz, "%ld of %ldkT: %08lx %d %s %c %%", (LONG)-1234567890,
                    (LONG)INT32_MAX, (DWORD)0x89abcdef, 17, "cargo", 'X') == 50);
    CHECK(!strcmp(sz, "-1234567890 of 2147483647kT: 89abcdef 17 cargo X %"));
    CHECK(wsprintfA(sz, "[%6.4d][%-5s][%#x][%hu]", 12, "ab", 42u, (WORD)1) > 0);
    CHECK(!strcmp(sz, "[  0012][ab   ][0x2a][1]"));
    CHECK(wsprintfA(sz, "%.3s %hd %lu", "fleet", (SHORT)-1, (DWORD)UINT32_MAX) > 0);
    CHECK(!strcmp(sz, "fle -1 4294967295"));
#ifdef STARS_UI_SDL
    // The owned C++ interface carries capacity; SDK comparisons above fit.
    char rgch[12];
    memset(rgch, '!', sizeof(rgch));
    CHECK(wsprintfA(std::span{rgch}.subspan(2, 5), "%s", "ab\xc3\xa9z") == 4);
    CHECK(!strcmp(rgch + 2, "ab\xc3\xa9") && rgch[1] == '!' && rgch[7] == '!');
    memset(rgch, '!', sizeof(rgch));
    CHECK(wsprintfA(std::span{rgch}.subspan(2, 4), "%s", "ab\xc3\xa9z") == 2);
    CHECK(!strcmp(rgch + 2, "ab") && rgch[1] == '!' && rgch[6] == '!');
    CHECK(wsprintfA(std::span{rgch}.subspan(2, 1), "%d", 12345) == 0 && rgch[2] == 0 && rgch[3] == 'b');
    rgch[2] = '!';
    CHECK(wsprintfA(std::span<char>{}, "%d", 12345) == 0 && rgch[2] == '!');
#endif
    CHECK(_stricmp("Fleet", "FLEET") == 0 && _stricmp("a", "b") < 0);
    CHECK(_strnicmp("Fleet one", "FLEET two", 5) == 0);
    CHECK(_strnicmp("a", "b", 0) == 0);
    CHECK(!strcmp(_strlwr(lstrcpyA(sz, "FL33T")), "fl33t"));
    CHECK(!strcmp(itoa(INT32_MIN, sz, 10), "-2147483648"));
    CHECK(!strcmp(itoa(-1, sz, 16), "ffffffff"));
    CHECK(!strcmp(itoa(35, sz, 36), "z"));
    CHECK(_strdate(szDate) == szDate && strlen(szDate) == 8 && szDate[2] == '/' && szDate[5] == '/');
    CHECK(_strtime(szTime) == szTime && strlen(szTime) == 8 && szTime[2] == ':' && szTime[5] == ':');
    CHECK(_getcwd(sz, sizeof(sz)) == sz && *sz);
    snprintf(szPath, sizeof(szPath), "stars-compat-services-%lu", (unsigned long)idThread);
    snprintf(szFile, sizeof(szFile), "%s/data", szPath);
    CHECK(_access(szPath, 0) == -1 && _mkdir(szPath) == 0 && _mkdir(szPath) == -1);
    CHECK(_access(szPath, 0) == 0);
    fp = fopen(szFile, "wb");
    CHECK(fp && fwrite("test", 1, 4, fp) == 4 && fclose(fp) == 0);
    CHECK(_access(szFile, 0) == 0 && _access(szFile, 2) == 0);
    CHECK(remove(szFile) == 0);
#ifdef _WIN32
    CHECK(_rmdir(szPath) == 0);
#else
    CHECK(rmdir(szPath) == 0);
    StarsCompatShutdown();
#endif
    puts("UI services: PASS (MulDiv, 32-bit formatting, strings, CRT paths/time)");
    return 0;
}
