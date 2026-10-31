// Native Windows process services are isolated from game and wrapper types.
#include "native_process.h"
#include "ownership.h"
#include <windows.h>
#include <shellapi.h>

char *PszNativeCommandLine(void) {
    const WCHAR *pwsz = GetCommandLineW();
    char *psz;
    int cb;
    if (*pwsz == L'"') {
        for (pwsz++; *pwsz && *pwsz != L'"'; pwsz++) {}
        if (*pwsz) pwsz++;
    } else {
        while (*pwsz && *pwsz != L' ' && *pwsz != L'\t') pwsz++;
    }
    while (*pwsz == L' ' || *pwsz == L'\t') pwsz++;
    cb = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pwsz, -1, NULL, 0, NULL, NULL);
    if (cb == 0 || !(psz = (char *)malloc(cb))) return NULL;
    NATIVEOWNER<char, std::free> command(psz);
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pwsz, -1, psz, cb, NULL, NULL)) {
        return NULL;
    }
    return command.release();
}

char **RgpszNativeArguments(int *lpc) {
    WCHAR **rgpwsz = CommandLineToArgvW(GetCommandLineW(), lpc);
    char **rgpsz, *pch;
    size_t cb;
    int i, c;
    NATIVEOWNER<WCHAR *, LocalFree> wideArguments(rgpwsz);
    if (!rgpwsz) return NULL;
    cb = ((size_t)*lpc + 1) * sizeof(*rgpsz);
    for (i = 0; i < *lpc; i++) {
        c = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, rgpwsz[i], -1, NULL, 0, NULL, NULL);
        if (c == 0) { wideArguments.reset(); return NULL; }
        cb += c;
    }
    rgpsz = (char **)malloc(cb);
    if (!rgpsz) { wideArguments.reset(); return NULL; }
    NATIVEOWNER<char *, std::free> arguments(rgpsz);
    pch = (char *)(rgpsz + *lpc + 1);
    for (i = 0; i < *lpc; i++) {
        c = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, rgpwsz[i], -1, NULL, 0, NULL, NULL);
        rgpsz[i] = pch;
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, rgpwsz[i], -1, pch, c, NULL, NULL);
        pch += c;
    }
    rgpsz[*lpc] = NULL; wideArguments.reset();
    return arguments.release();
}

uint32_t DwTickCount() { return GetTickCount(); }

