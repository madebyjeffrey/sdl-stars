#ifndef STARS_DECOMPILED_MSGUI_H
#define STARS_DECOMPILED_MSGUI_H

#include <stdint.h>
#include <windows.h>

LRESULT CALLBACK MessageWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DecorateMsgTitleBar(HDC hdc, RECT *prc);
HtMsgType        HtMsgBox(POINT16 pt);
void             SetMsgTitle(HWND hwnd);

#endif
