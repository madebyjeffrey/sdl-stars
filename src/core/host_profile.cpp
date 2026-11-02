#include "host_profile.h"
#include "text_io.h"
#include "encoding.h"
#include <algorithm>

// FSzEqNoCase compares two strings, ignoring case, as INI names are.
static bool FSzEqNoCase(std::string_view sz1, std::string_view sz2) {
    std::size_t i;

    if (sz1.size() != sz2.size())
        return false;
    for (i = 0; i < sz1.size(); i++) {
        if (std::tolower(static_cast<unsigned char>(sz1[i])) != std::tolower(static_cast<unsigned char>(sz2[i])))
            return false;
    }
    return true;
}

// FIniValue copies key szKey's value in [szSection] of the INI text szText
// into szValue, as GetPrivateProfileString finds it.
bool FIniValue(APPSESSION &app, std::string_view szText, StringId idsSection, StringId idsKey, std::span<char> szValue) {
    std::array<char, 32> szSection, szKey;
    std::size_t ichLine, ichLineEnd, ichClose, ichEquals, cch;
    std::int16_t cchSection, cchKey;
    bool fInSection;
    std::string_view szLine, szName, szValueText;

    if (szValue.empty())
        return false;
    cchSection = TextIO().pfnCchGetString(app, idsSection, std::span{szSection});
    cchKey = TextIO().pfnCchGetString(app, idsKey, std::span{szKey});
    fInSection = false;
    for (ichLine = 0; ichLine < szText.size(); ichLine = ichLineEnd < szText.size() ? ichLineEnd + 1 : szText.size()) {
        ichLineEnd = szText.find('\n', ichLine);
        if (ichLineEnd == std::string_view::npos)
            ichLineEnd = szText.size();
        szLine = szText.substr(ichLine, ichLineEnd - ichLine);
        cch = CchUtf8Prefix(szLine, std::min<std::size_t>(szLine.size(), 255));
        szLine = szLine.substr(0, cch);
        while (!szLine.empty() && (szLine.back() == '\r' || szLine.back() == ' ' || szLine.back() == '\t'))
            szLine.remove_suffix(1);
        if (szLine.empty())
            continue;
        if (szLine.front() == '[') {
            ichClose = szLine.find(']');
            if (ichClose != std::string_view::npos)
                fInSection = FSzEqNoCase(szLine.substr(1, ichClose - 1), std::string_view{szSection.data(), static_cast<std::size_t>(cchSection)});
            continue;
        }
        ichEquals = szLine.find('=');
        if (!fInSection || ichEquals == std::string_view::npos)
            continue;
        szName = szLine.substr(0, ichEquals);
        while (!szName.empty() && (szName.back() == ' ' || szName.back() == '\t'))
            szName.remove_suffix(1);
        if (FSzEqNoCase(szName, std::string_view{szKey.data(), static_cast<std::size_t>(cchKey)})) {
            szValueText = szLine.substr(ichEquals + 1);
            while (!szValueText.empty() && (szValueText.front() == ' ' || szValueText.front() == '\t'))
                szValueText.remove_prefix(1);
            CchCopyUtf8Prefix(szValue, szValueText);
            return true;
        }
    }
    return false;
}

// LIniInt reads an INI key as GetPrivateProfileInt does: its leading
// number, or lDefault if the key is missing.
std::int32_t LIniInt(APPSESSION &app, std::string_view szText, StringId idsSection, StringId idsKey, std::int32_t lDefault) {
    std::array<char, 32> szValue;

    if (!FIniValue(app, szText, idsSection, idsKey, szValue)) {
        return lDefault;
    }
    return std::atol(szValue.data());
}
