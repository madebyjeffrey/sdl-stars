/* Menu ownership, recursive commands and return values run against the SDK too. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
int main(void) {
    HMENU hmenu, hsub, hnested, hdetach;
    char sz[32], szLabel[] = "&First\tCtrl+F";
    hmenu = CreateMenu(); hsub = CreatePopupMenu(); hnested = CreatePopupMenu();
    CHECK(hmenu && hsub && hnested && IsMenu(hmenu));
    CHECK(GetMenuItemCount(hmenu) == 0);
    CHECK(AppendMenu(hmenu, MF_STRING, 10, szLabel));
    szLabel[1] = 'X';
    CHECK(GetMenuString(hmenu, 10, sz, sizeof(sz), MF_BYCOMMAND) == 13);
    CHECK(strcmp(sz, "&First\tCtrl+F") == 0);
    CHECK(AppendMenu(hmenu, MF_SEPARATOR, 999, NULL));
    CHECK(GetMenuItemID(hmenu, 1) == 999);
    CHECK(GetMenuState(hmenu, 1, MF_BYPOSITION) & MF_SEPARATOR);
    CHECK(AppendMenu(hnested, MF_CHECKED | MF_GRAYED, 42, "Nested"));
    CHECK(AppendMenu(hsub, MF_POPUP, (UINT_PTR)hnested, "More"));
    CHECK(AppendMenu(hmenu, MF_POPUP, (UINT_PTR)hsub, "Sub"));
    CHECK(GetSubMenu(hmenu, 2) == hsub && GetSubMenu(hsub, 0) == hnested);
    CHECK(GetSubMenu(hmenu, -1) == NULL && GetSubMenu(hmenu, 3) == NULL);
    CHECK(GetMenuItemID(hmenu, 2) == (UINT)-1);
    CHECK(GetMenuString(hnested, 42, sz, 4, MF_BYCOMMAND) == 3 && memcmp(sz, "Nes", 3) == 0);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    /* Wine 9 writes the fourth character rather than the expected terminator. */
    CHECK(sz[3] == 0);
#endif
    CHECK(GetMenuString(hnested, 42, sz, sizeof(sz), MF_BYCOMMAND) == 6 && strcmp(sz, "Nested") == 0);
    CHECK(GetMenuString(hnested, 42, NULL, 0, MF_BYCOMMAND) == 6);
    CHECK(CheckMenuItem(hmenu, 42, MF_UNCHECKED) == MF_CHECKED);
    CHECK(CheckMenuItem(hmenu, 42, MF_CHECKED) == MF_UNCHECKED);
    CHECK(EnableMenuItem(hmenu, 42, MF_DISABLED) == MF_GRAYED);
    CHECK(EnableMenuItem(hmenu, 42, MF_ENABLED) == MF_DISABLED);
    CHECK((GetMenuState(hnested, 0, MF_BYPOSITION) & (MF_CHECKED | MF_DISABLED | MF_GRAYED)) == MF_CHECKED);
    CHECK(CheckMenuItem(hmenu, 555, MF_CHECKED) == (DWORD)-1);
    CHECK(EnableMenuItem(hmenu, 555, MF_GRAYED) == -1);
    CHECK(InsertMenu(hmenu, 42, MF_BYCOMMAND, 41, "Before"));
    CHECK(GetMenuItemCount(hnested) == 2 && GetMenuItemID(hnested, 0) == 41);
    CHECK(InsertMenu(hmenu, (UINT)-1, MF_BYPOSITION, 20, "Last"));
    CHECK(GetMenuItemID(hmenu, 3) == 20);
    CHECK(DeleteMenu(hmenu, 41, MF_BYCOMMAND) && GetMenuItemCount(hnested) == 1);
    CHECK(DeleteMenu(hmenu, 1, MF_BYPOSITION) && GetMenuItemCount(hmenu) == 3);
    CHECK(RemoveMenu(hmenu, 1, MF_BYPOSITION));
    CHECK(IsMenu(hsub) && IsMenu(hnested));
    CHECK(AppendMenu(hmenu, MF_POPUP, (UINT_PTR)hsub, "Reattach"));
    CHECK(DeleteMenu(hmenu, 2, MF_BYPOSITION));
    CHECK(!IsMenu(hsub) && !IsMenu(hnested));
    hdetach = CreatePopupMenu();
    CHECK(AppendMenu(hmenu, MF_POPUP, (UINT_PTR)hdetach, "Detached"));
    CHECK(RemoveMenu(hmenu, 2, MF_BYPOSITION));
    CHECK(DestroyMenu(hmenu) && IsMenu(hdetach));
    CHECK(DestroyMenu(hdetach));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(!DestroyMenu(hmenu) && GetMenuItemCount(hmenu) == -1);
    CHECK(!IsMenu((HMENU)(UINT_PTR)1234));
    hmenu = CreateMenu(); hsub = CreatePopupMenu();
    CHECK(AppendMenu(hmenu, MF_POPUP, (UINT_PTR)hsub, "Sub"));
    CHECK(!AppendMenu(hsub, MF_POPUP, (UINT_PTR)hmenu, "Cycle"));
    CHECK(!AppendMenu(hmenu, MF_POPUP, (UINT_PTR)hsub, "Duplicate"));
    CHECK(!InsertMenu(hmenu, 900, MF_BYCOMMAND, 20, "Missing"));
    CHECK(GetMenuItemCount(hmenu) == 1 && GetMenuItemCount(hsub) == 0);
    CHECK(DestroyMenu(hmenu));
    /* Shutdown releases unclaimed trees and supports a fresh runtime. */
    hmenu = CreatePopupMenu();
    CHECK(AppendMenu(hmenu, MF_STRING, 1, "Shutdown"));
    StarsCompatShutdown();
    hmenu = CreateMenu(); CHECK(hmenu && DestroyMenu(hmenu));
    /* Attach a deep tree from its root; depth checks include descendants. */
    {
        HMENU rgmenu[65];
        int i;
        for (i = 0; i < 65; i++) { rgmenu[i] = CreatePopupMenu(); CHECK(rgmenu[i]); }
        for (i = 62; i >= 0; i--)
            CHECK(AppendMenu(rgmenu[i], MF_POPUP, (UINT_PTR)rgmenu[i + 1], "Depth"));
        CHECK(!AppendMenu(rgmenu[64], MF_POPUP, (UINT_PTR)rgmenu[0], "Too deep"));
        CHECK(GetMenuItemCount(rgmenu[64]) == 0);
        CHECK(DestroyMenu(rgmenu[0]) && DestroyMenu(rgmenu[64]));
    }
    StarsCompatShutdown();
#endif
    puts("Menu data tests passed");
    return 0;
}
