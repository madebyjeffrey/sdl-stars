#ifndef STARS_DECOMPILED_MSGUI_H
#define STARS_DECOMPILED_MSGUI_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

LRESULT  MessageWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DecorateMsgTitleBar(APPSESSION &app, HDC hdc, RECT *prc);
HtMsgType        HtMsgBox(APPSESSION &app, POINT16 pt);
void             SetMsgTitle(APPSESSION &app, HWND hwnd);

#endif
