// Native Windows process services are isolated from game and wrapper types.
#include "native_process.h"
#include "ownership.h"
#include <new>
#include <utility>
#include <windows.h>
#include <shellapi.h>

std::optional<std::vector<std::string>> RgNativeArguments() {
    int cArgs;
    int cch;
    WCHAR **rgpwsz = CommandLineToArgvW(GetCommandLineW(), &cArgs);
    NATIVEOWNER<WCHAR *, LocalFree> wideArguments(rgpwsz);

    if (!rgpwsz || cArgs < 0) return std::nullopt;
    try {
        std::vector<std::string> rgsz;
        rgsz.reserve(static_cast<std::size_t>(cArgs));
        for (int i = 0; i < cArgs; i++) {
            cch = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, rgpwsz[i], -1, nullptr, 0, nullptr, nullptr);
            if (cch <= 0) return std::nullopt;
            std::string sz(static_cast<std::size_t>(cch), '\0');
            if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, rgpwsz[i], -1, sz.data(), cch, nullptr, nullptr)) return std::nullopt;
            sz.pop_back();
            rgsz.push_back(std::move(sz));
        }
        return rgsz;
    } catch (const std::bad_alloc &) {
        return std::nullopt;
    }
}

std::uint32_t DwTickCount() { return GetTickCount(); }

