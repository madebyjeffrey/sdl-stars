#ifndef STARS_WIN_CALLS_H
#define STARS_WIN_CALLS_H

#include <windows.h>
#include "errors.h"
// Callback exceptions are captured inside runtime callbacks. Deliver them only
// after that call completes its dispatch, modal and owner cleanup.
template <auto fn> struct StarsCFunction;
template <class R, class... Args, R (*fn)(Args...)> struct StarsCFunction<fn> {
    static R Call(Args... args) {
        if constexpr (std::is_void<R>::value) {
            fn(args...);
            StarsRethrowPendingError();
        } else {
            R result = fn(args...);
            StarsRethrowPendingError();
            return result;
        }
    }
};

// Capture the original C symbol (including SDK namespaces and test adapters)
// before redirecting source calls to the C++ delivery boundary.
#define STARS_CPP_CALL(name) using StarsCall_##name = StarsCFunction<&name>;

STARS_CPP_CALL(CallWindowProcA)
#undef CallWindowProcA
#define CallWindowProcA StarsCall_CallWindowProcA::Call

STARS_CPP_CALL(CreateWindowExA)
#undef CreateWindowExA
#define CreateWindowExA StarsCall_CreateWindowExA::Call

STARS_CPP_CALL(CreateDialogParamA)
#undef CreateDialogParamA
#define CreateDialogParamA StarsCall_CreateDialogParamA::Call

STARS_CPP_CALL(DialogBoxParamA)
#undef DialogBoxParamA
#define DialogBoxParamA StarsCall_DialogBoxParamA::Call

STARS_CPP_CALL(DestroyWindow)
#undef DestroyWindow
#define DestroyWindow StarsCall_DestroyWindow::Call

STARS_CPP_CALL(DispatchMessageA)
#undef DispatchMessageA
#define DispatchMessageA StarsCall_DispatchMessageA::Call

STARS_CPP_CALL(GetMessageA)
#undef GetMessageA
#define GetMessageA StarsCall_GetMessageA::Call

STARS_CPP_CALL(PeekMessageA)
#undef PeekMessageA
#define PeekMessageA StarsCall_PeekMessageA::Call

STARS_CPP_CALL(SendMessageA)
#undef SendMessageA
#define SendMessageA StarsCall_SendMessageA::Call

STARS_CPP_CALL(SendDlgItemMessageA)
#undef SendDlgItemMessageA
#define SendDlgItemMessageA StarsCall_SendDlgItemMessageA::Call

STARS_CPP_CALL(ShowWindow)
#undef ShowWindow
#define ShowWindow StarsCall_ShowWindow::Call

STARS_CPP_CALL(EnableWindow)
#undef EnableWindow
#define EnableWindow StarsCall_EnableWindow::Call

STARS_CPP_CALL(SetFocus)
#undef SetFocus
#define SetFocus StarsCall_SetFocus::Call

STARS_CPP_CALL(SetWindowPos)
#undef SetWindowPos
#define SetWindowPos StarsCall_SetWindowPos::Call

STARS_CPP_CALL(MoveWindow)
#undef MoveWindow
#define MoveWindow StarsCall_MoveWindow::Call

STARS_CPP_CALL(UpdateWindow)
#undef UpdateWindow
#define UpdateWindow StarsCall_UpdateWindow::Call

STARS_CPP_CALL(SetWindowTextA)
#undef SetWindowTextA
#define SetWindowTextA StarsCall_SetWindowTextA::Call

STARS_CPP_CALL(IsDialogMessageA)
#undef IsDialogMessageA
#define IsDialogMessageA StarsCall_IsDialogMessageA::Call

STARS_CPP_CALL(TranslateAcceleratorA)
#undef TranslateAcceleratorA
#define TranslateAcceleratorA StarsCall_TranslateAcceleratorA::Call

STARS_CPP_CALL(TrackPopupMenu)
#undef TrackPopupMenu
#define TrackPopupMenu StarsCall_TrackPopupMenu::Call

STARS_CPP_CALL(DefWindowProcA)
#undef DefWindowProcA
#define DefWindowProcA StarsCall_DefWindowProcA::Call

STARS_CPP_CALL(GetOpenFileNameA)
#undef GetOpenFileNameA
#define GetOpenFileNameA StarsCall_GetOpenFileNameA::Call

STARS_CPP_CALL(GetSaveFileNameA)
#undef GetSaveFileNameA
#define GetSaveFileNameA StarsCall_GetSaveFileNameA::Call

#undef STARS_CPP_CALL
#endif
