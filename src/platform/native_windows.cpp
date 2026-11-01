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
    cb = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pwsz, -1, nullptr, 0, nullptr, nullptr);
    if (cb == 0 || !(psz = static_cast<char *>(std::malloc(cb)))) return nullptr;
    NATIVEOWNER<char, std::free> command(psz);
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pwsz, -1, psz, cb, nullptr, nullptr)) {
        return nullptr;
    }
    return command.release();
}

char **RgpszNativeArguments(int *lpc) {
    WCHAR **rgpwsz = CommandLineToArgvW(GetCommandLineW(), lpc);
    char **rgpsz, *pch;
    std::size_t cb;
    int i, c;
    NATIVEOWNER<WCHAR *, LocalFree> wideArguments(rgpwsz);
    if (!rgpwsz) return nullptr;
    cb = (static_cast<std::size_t>(*lpc) + 1) * sizeof(*rgpsz);
    for (i = 0; i < *lpc; i++) {
        c = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, rgpwsz[i], -1, nullptr, 0, nullptr, nullptr);
        if (c == 0) { wideArguments.reset(); return nullptr; }
        cb += c;
    }
    rgpsz = static_cast<char **>(std::malloc(cb));
    if (!rgpsz) { wideArguments.reset(); return nullptr; }
    NATIVEOWNER<char *, std::free> arguments(rgpsz);
    pch = reinterpret_cast<char *>(rgpsz + *lpc + 1);
    for (i = 0; i < *lpc; i++) {
        c = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, rgpwsz[i], -1, nullptr, 0, nullptr, nullptr);
        rgpsz[i] = pch;
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, rgpwsz[i], -1, pch, c, nullptr, nullptr);
        pch += c;
    }
    rgpsz[*lpc] = nullptr; wideArguments.reset();
    return arguments.release();
}

std::uint32_t DwTickCount() { return GetTickCount(); }

