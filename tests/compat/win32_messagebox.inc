#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include "../../compat/win32/src/audio_io.h"
#endif
/* Real modal prompts driven by ordinary owner timers, also checked with SDK. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#include "internal.h"
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
#define EXPECT(f) do { if (!(f)) { fprintf(stderr, "callback line %d: %s\n", __LINE__, #f); cErrors++; } } while (0)
static HWND hwndOwner, hwndOther, hwndDisabled, hwndExpectedOwner;
static int mode, idChoose, idDefault, stage, cErrors, cSeen;
static BOOL fTask, fDone;
#if !defined(_WIN32) || defined(STARS_UI_SDL)
static BOOL fDisableFailure;
static int cAudioQueues;
bool SDL_PutAudioStreamData(SDL_AudioStream *lp, const void *lpv, int cb);
extern "C" bool SDLCALL StarsTest_SDL_PutAudioStreamData(SDL_AudioStream *lp, const void *lpv, int cb) {
    SDL_AudioSpec spec;
    const float *rg = (const float *)lpv;
    int i;
    BOOL fAudible = 0;
    EXPECT(SDL_GetAudioStreamFormat(lp, &spec, NULL));
    EXPECT(spec.format == SDL_AUDIO_F32 && spec.channels == 1 && spec.freq > 0);
    EXPECT(lpv && cb > 0 && cb % (int)sizeof(float) == 0);
    if (lpv && cb > 0) for (i = 0; i < cb / (int)sizeof(float); i++) {
        EXPECT(isfinite(rg[i]) && fabsf(rg[i]) <= 1.0f);
        if (fabsf(rg[i]) > 0.01f) fAudible = 1;
    }
    EXPECT(fAudible); cAudioQueues++;
    return SDL_PutAudioStreamData(lp, lpv, cb);
}
#endif
static HWND HwndBox(void) {
    HWND hwnd;
    char sz[64] = {0};
    for (hwnd = GetWindow(hwndOwner, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
        GetWindowTextA(hwnd, sz, sizeof(sz));
        if (strcmp(sz, "Stars Message Contract") == 0 && IsWindowVisible(hwnd)) return hwnd;
    }
    return NULL;
}
static LRESULT CALLBACK OwnerProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND hbox, hbutton, hbody, hscroll;
    APPMSG message;
    char sz[64] = {0};
    int nested;
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    if (msg == WM_ENABLE && !w && fDisableFailure) StarsThrowError(starsErrorFile, 836);
#endif
    if (msg == WM_TIMER && !fDone && (hbox = HwndBox())) {
        if (!stage) {
            cSeen++;
            EXPECT(GetParent(hbox) == hwndExpectedOwner);
            EXPECT(!IsWindowEnabled(hwndOwner) && !IsWindowEnabled(hwndDisabled));
            EXPECT(IsWindowEnabled(hwndOther) == !fTask);
            EXPECT(GetDlgItem(hbox, idChoose));
            if (GetFocus() != GetDlgItem(hbox, idDefault)) {
                GetWindowTextA(GetFocus(), sz, sizeof(sz));
                fprintf(stderr, "mode %d stage %d: focus id %d title %s, default %d, box focus %d, active box %d\n", mode, stage, GetDlgCtrlID(GetFocus()), sz, idDefault, GetFocus() == hbox, GetActiveWindow() == hbox);
            }
            EXPECT(GetFocus() == GetDlgItem(hbox, idDefault));
        }
        stage++;
        hbutton = GetFocus();
        if (mode == 3 && stage == 1) { PostMessageA(hbutton, WM_KEYDOWN, VK_ESCAPE, 0); return 0; }
        if (mode == 4) { SendMessageA(hbox, WM_CLOSE, 0, 0); EXPECT(IsWindow(hbox)); }
        if (mode == 6) {
            mode = 0; stage = 0; idChoose = IDOK; idDefault = IDOK; hwndExpectedOwner = hbox;
            nested = MessageBoxA(hbox, "Inner prompt", "Stars Message Contract", MB_OK);
            EXPECT(nested == IDOK && IsWindowEnabled(hbox) && !IsWindowEnabled(hwndOwner));
            fDone = 0; idChoose = IDYES;
        }
        fDone = 1;
#if !defined(_WIN32) || defined(STARS_UI_SDL)
        if (mode == 10) { EXPECT(DestroyWindow(hwndOwner)); return 0; }
        if (mode == 9) {
            hbody = GetDlgItem(hbox, 0xffff); hscroll = GetDlgItem(hbody, 1);
            EXPECT(hbody && hscroll);
            SendMessageA(hbody, WM_VSCROLL, SB_BOTTOM, (LPARAM)hscroll);
            EXPECT(GetScrollPos(hscroll, SB_CTL) > 0);
            SendMessageA(hbody, WM_KEYDOWN, VK_HOME, 0); EXPECT(GetScrollPos(hscroll, SB_CTL) == 0);
            SendMessageA(hbody, WM_KEYDOWN, VK_NEXT, 0); EXPECT(GetScrollPos(hscroll, SB_CTL) > 0);
            UpdateWindow(hbody);
        }
#else
        (void)hbody; (void)hscroll;
#endif
        if (mode == 1 || mode == 2 || mode == 5) {
            if (mode == 5) PostMessageA(hbutton, WM_KEYDOWN, VK_TAB, 0);
            PostMessageA(hbutton, WM_KEYDOWN, mode == 2 ? VK_ESCAPE : VK_RETURN, 0);
        } else if (mode == 7) {
            message = (APPMSG){0}; message.hwnd = GetFocus(); message.message = WM_KEYDOWN; message.wParam = VK_RETURN;
            EXPECT(IsDialogMessageA(hbox, &message));
        } else if (mode == 8) {
            GetWindowTextA(GetDlgItem(hbox, idChoose), sz, sizeof(sz));
            PostMessageA(hbox, WM_SYSCHAR, 'n', 0);
        } else SendMessageA(hbox, WM_COMMAND, MAKEWPARAM(idChoose, BN_CLICKED), (LPARAM)GetDlgItem(hbox, idChoose));
        return 0;
    }
    return DefWindowProcA(hwnd, msg, w, l);
}
static int IdRun(UINT type, int id, int idDef, int md, BOOL task) {
    int result;
    mode = md; idChoose = id; idDefault = idDef; stage = 0; fDone = 0; fTask = task;
    hwndExpectedOwner = task ? NULL : hwndOwner;
    if (!SetTimer(hwndOwner, 91, 20, NULL)) return -1;
    result = MessageBoxA(task ? NULL : md == 11 ? GetDlgItem(hwndOwner, 20) : hwndOwner,
                        "First line\nSecond line with &literal ampersand.", "Stars Message Contract", type);
    KillTimer(hwndOwner, 91);
    printf("Message type 0x%x mode %d result %d\n", type, md, result);
    if (!IsWindowEnabled(hwndOwner) || !IsWindowEnabled(hwndOther) || IsWindowEnabled(hwndDisabled)) cErrors++;
    return result;
}
int main(int argc, char **argv) {
    WNDCLASS wc = {0};
    HWND hwndEdit;
    APPMSG msg;
    int before;
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    char szLong[8192];
    HWND hwndSave;
    int i;
    COMPATAUDIOIO io_compataudioio;
    io_compataudioio.pfnPutAudioStreamData = StarsTest_SDL_PutAudioStreamData;
    COMPATAUDIOIOSCOPE scope_compataudioio(io_compataudioio);
    if (argc == 2 && strcmp(argv[1], "--audio-failure") == 0) {
        CHECK(!MessageBeep(MB_OK) && GetLastError() == ERROR_NOT_SUPPORTED);
        CHECK(!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO));
        StarsCompatShutdown(); puts("Unavailable audio returns failure without retaining the subsystem"); return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--external-audio") == 0) {
        CHECK(SDL_InitSubSystem(SDL_INIT_AUDIO) && MessageBeep(MB_OK));
        CHECK(cAudioQueues == 1 && cErrors == 0);
        StarsCompatShutdown(); CHECK(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO);
        SDL_QuitSubSystem(SDL_INIT_AUDIO); puts("External SDL audio ownership preserved"); return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--audio-muted") == 0) {
        CHECK(MessageBeep(MB_OK) && cAudioQueues == 0);
        CHECK(!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO));
        StarsCompatShutdown(); puts("Muted alerts need no audio device"); return 0;
    }
#else
    (void)argc; (void)argv;
#endif
    wc.lpfnWndProc = OwnerProc; wc.lpszClassName = "MessageOwner";
    CHECK(RegisterClassA(&wc));
    hwndOwner = CreateWindowExA(0, wc.lpszClassName, "Owner", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 40, 40, 500, 300, NULL, NULL, NULL, NULL);
    hwndOther = CreateWindowExA(0, wc.lpszClassName, "Other", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 80, 80, 250, 180, NULL, NULL, NULL, NULL);
    hwndDisabled = CreateWindowExA(0, wc.lpszClassName, "Disabled", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_DISABLED, 120, 120, 250, 180, NULL, NULL, NULL, NULL);
    CHECK(hwndOwner && hwndOther && hwndDisabled);
    hwndEdit = CreateWindowExA(0, "Edit", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 4, 4, 100, 22, hwndOwner, (HMENU)20, NULL, NULL); CHECK(hwndEdit);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageA(&msg); }
    SetFocus(hwndEdit);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(StarsCompatSetWindowAlertSounds(hwndOwner, 0));
    CHECK(IdRun(MB_YESNO | MB_ICONQUESTION, IDYES, IDYES, 11, 0) == IDYES);
    CHECK(IdRun(MB_YESNO | MB_ICONQUESTION, IDYES, IDYES, 6, 0) == IDYES);
    CHECK(cAudioQueues == 0 && !(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO));
    CHECK(StarsCompatSetWindowAlertSounds(hwndOther, 0));
    CHECK(StarsCompatSetWindowAlertSounds(hwndOwner, 1));
    CHECK(IdRun(MB_OK | MB_ICONASTERISK, IDOK, IDOK, 0, 0) == IDOK);
    CHECK(cAudioQueues == 1);
    CHECK(StarsCompatSetWindowAlertSounds(hwndOther, 1));
    cSeen = 0;
#endif
    CHECK(IdRun(MB_OK | MB_ICONASTERISK, IDOK, IDOK, 0, 0) == IDOK);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(GetFocus() == hwndEdit);
#else
    printf("SDK focus restored to original edit: %s\n", GetFocus() == hwndEdit ? "yes" : "no");
#endif
    CHECK(IdRun(MB_OKCANCEL, IDCANCEL, IDOK, 0, 0) == IDCANCEL);
    CHECK(IdRun(MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL | MB_DEFBUTTON2, IDNO, IDNO, 1, 0) == IDNO);
    CHECK(IdRun(MB_YESNOCANCEL | MB_ICONEXCLAMATION, IDCANCEL, IDYES, 2, 0) == IDCANCEL);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(IdRun(MB_OK, IDOK, IDOK, 2, 0) == IDOK);
#else
    before = IdRun(MB_OK, IDOK, IDOK, 2, 0);
    printf("SDK OK-only Escape result: %d (documented IDOK=%d)\n", before, IDOK);
    CHECK(before == IDOK || before == IDCANCEL);
#endif
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(IdRun(MB_YESNO, IDNO, IDYES, 3, 0) == IDNO);
#else
    before = IdRun(MB_YESNO, IDNO, IDYES, 3, 0);
    printf("SDK Yes/No Escape result: %d (documented Escape ignored)\n", before);
    CHECK(before == IDNO || before == IDCANCEL);
#endif
    CHECK(IdRun(MB_YESNO, IDNO, IDYES, 4, 0) == IDNO);
    CHECK(IdRun(MB_YESNO, IDNO, IDYES, 5, 0) == IDNO);
    CHECK(IdRun(MB_ABORTRETRYIGNORE | MB_DEFBUTTON3, IDIGNORE, IDIGNORE, 1, 0) == IDIGNORE);
    CHECK(IdRun(MB_RETRYCANCEL, IDRETRY, IDRETRY, 0, 0) == IDRETRY);
    CHECK(IdRun(MB_CANCELTRYCONTINUE | MB_DEFBUTTON2, IDTRYAGAIN, IDTRYAGAIN, 1, 0) == IDTRYAGAIN);
    CHECK(IdRun(MB_OK | MB_TASKMODAL, IDOK, IDOK, 0, 1) == IDOK);
    CHECK(IdRun(MB_YESNO, IDYES, IDYES, 6, 0) == IDYES);
    CHECK(cErrors == 0 && cSeen == 14);
    CHECK(IdRun(MB_YESNO, IDNO, IDYES, 8, 0) == IDNO);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    before = cAudioQueues;
    CHECK(MessageBeep(MB_OK) && MessageBeep(MB_ICONHAND) && MessageBeep(MB_ICONQUESTION) &&
          MessageBeep(MB_ICONEXCLAMATION) && MessageBeep(MB_ICONASTERISK) && MessageBeep(UINT32_MAX));
    CHECK(cAudioQueues == before + 6);
    CHECK(!MessageBeep(3) && GetLastError() == ERROR_INVALID_PARAMETER);
    CHECK(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO);
    before = cSeen; CHECK(!MessageBoxA(hwndOwner, "Unsupported", "Stars Message Contract", 0x00200000)); CHECK(cSeen == before);
    PostQuitMessage(123); CHECK(!MessageBoxA(hwndOwner, "Quit pending", "Stars Message Contract", MB_OK));
    CHECK(PeekMessageA(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE) && msg.wParam == 123);
    CHECK(IsWindowEnabled(hwndOwner));
    for (i = 0; i < (int)sizeof(szLong) - 1; i++) szLong[i] = i % 40 == 39 ? '\n' : 'x';
    szLong[sizeof(szLong) - 1] = 0;
    mode = 9; idChoose = IDOK; idDefault = IDOK; stage = 0; fDone = 0; fTask = 0; hwndExpectedOwner = hwndOwner;
    CHECK(SetTimer(hwndOwner, 91, 20, NULL)); CHECK(MessageBoxA(hwndOwner, szLong, "Stars Message Contract", MB_OK) == IDOK); KillTimer(hwndOwner, 91);
    EnableWindow(hwndOwner, 0); mode = 0; stage = 0; fDone = 0;
    CHECK(SetTimer(hwndOwner, 91, 20, NULL));
    CHECK(MessageBoxA(hwndOwner, "Initially disabled owner", "Stars Message Contract", MB_OK) == IDOK);
    KillTimer(hwndOwner, 91); CHECK(!IsWindowEnabled(hwndOwner)); EnableWindow(hwndOwner, 1);
    before = cAudioQueues; fDisableFailure = 1;
    CHECK(!MessageBoxA(hwndOwner, "Failed owner disable", "Stars Message Contract", MB_OK | MB_ICONHAND));
    CHECK(FStarsErrorPending() && IsWindowEnabled(hwndOwner) && cAudioQueues == before);
    CHECK(!MessageBoxA(hwndOwner, "Failure already pending", "Stars Message Contract", MB_OK | MB_ICONHAND));
    CHECK(IsWindowEnabled(hwndOwner) && cAudioQueues == before);
    try { StarsRethrowPendingError(); CHECK(0); }
    catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == 836); }
    fDisableFailure = 0;
    hwndSave = hwndOwner;
    hwndOwner = CreateWindowExA(0, wc.lpszClassName, "Destroyed owner", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              40, 40, 300, 200, NULL, NULL, NULL, NULL); CHECK(hwndOwner);
    hwndExpectedOwner = hwndOwner; mode = 10; stage = 0; fDone = 0;
    CHECK(SetTimer(hwndOwner, 91, 20, NULL));
    CHECK(!MessageBoxA(hwndOwner, "Owner teardown", "Stars Message Contract", MB_OK) && !IsWindow(hwndOwner));
    hwndOwner = hwndSave;
#else
    (void)before; (void)msg;
#endif
    CHECK(cErrors == 0 && DestroyWindow(hwndOwner) && DestroyWindow(hwndOther) && DestroyWindow(hwndDisabled));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    StarsCompatShutdown(); CHECK(!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO));
#endif
    puts("Message-box buttons, keyboard, modality, nesting and owner restoration passed"); return 0;
}
