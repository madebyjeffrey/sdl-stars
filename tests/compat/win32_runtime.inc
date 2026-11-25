/* Run the same geometry cases on Linux and the real Windows implementation. */
#include <windows.h>
#include <stdio.h>

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
#ifndef _WIN32
int FTestTickCount(void);
#endif

int main(void) {
    RECT rc1, rc2, rcDst, rcWant;
    POINT pt;

    CHECK(SetRect(&rc1, -10, -20, 30, 40));
    CHECK(CopyRect(&rc2, &rc1) && EqualRect(&rc1, &rc2));
    CHECK(CopyRect(&rc1, &rc1));
    pt.x = -10; pt.y = -20;
    CHECK(PtInRect(&rc1, pt));
    pt.x = 30;
    CHECK(!PtInRect(&rc1, pt));
    pt.x = 29; pt.y = 40;
    CHECK(!PtInRect(&rc1, pt));
    pt.y = 39;
    CHECK(PtInRect(&rc1, pt));
    CHECK(OffsetRect(&rc2, 10, 20));
    CHECK(SetRect(&rcWant, 0, 0, 40, 60) && EqualRect(&rc2, &rcWant));
    CHECK(InflateRect(&rc2, -5, -10));
    CHECK(SetRect(&rcWant, 5, 10, 35, 50) && EqualRect(&rc2, &rcWant));
    CHECK(IntersectRect(&rcDst, &rc1, &rc2));
    CHECK(SetRect(&rcWant, 5, 10, 30, 40) && EqualRect(&rcDst, &rcWant));
    CHECK(IntersectRect(&rc1, &rc1, &rc2) && EqualRect(&rc1, &rcWant));
    CHECK(IntersectRect(&rc2, &rc1, &rc2) && EqualRect(&rc2, &rcWant));
    CHECK(SetRect(&rc1, 30, 10, 35, 40));
    CHECK(!IntersectRect(&rcDst, &rc1, &rc2));
    CHECK(SetRect(&rcWant, 0, 0, 0, 0) && EqualRect(&rcDst, &rcWant));
    CHECK(SetRect(&rc1, 12, 12, 4, 4));
    CHECK(!IntersectRect(&rcDst, &rc1, &rc2) && EqualRect(&rcDst, &rcWant));
    pt.x = 5; pt.y = 5;
    CHECK(!PtInRect(&rc1, pt));
    CHECK(SetRect(&rc1, 0, 0, 0, 0) && !PtInRect(&rc1, pt));
    CHECK(InflateRect(&rc1, 2, 3));
    CHECK(SetRect(&rcWant, -2, -3, 2, 3) && EqualRect(&rc1, &rcWant));
#ifndef _WIN32
    CHECK(FTestTickCount());
#endif
    puts("Win32 runtime geometry cases passed");
    return 0;
}
