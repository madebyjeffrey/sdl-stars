#include "preferencesui.h"
#include <algorithm>
#include "appstate.h"
#include "uistate.h"
#include "profile_text_io.h"
#include "text_io.h"
#include "win.h"
#include <charconv>
#include <string>

void ReadIniPresets(APPSESSION &app, const char *szIniFile) {
    std::int16_t i, iPass, cch, cpq;
    std::size_t ich, cchPrefix;
    char    szSection[16], szEntry[16];
    std::string_view szPreset;

    TextIO().pfnCchGetString(app, idsZiporders, std::span{szSection});
    std::fill_n(app.state.vrgZip, 4, ZIPORDER{});
    for (i = 0; i < 4; i++) {
        CchSnprintf(std::span{szEntry}, "%s%d", szSection, i + 1);
        cch = ProfileTextIO().pfnReadString(szSection, szEntry, "", app.state.szWork, 80, szIniFile);
        szPreset = std::string_view{app.state.szWork, static_cast<std::size_t>(cch)};
        if (szPreset.size() >= 20 && szPreset.size() <= 32) {
            if (std::all_of(szPreset.begin(), szPreset.begin() + 20, [](char ch) { return ch >= 'a' && ch <= 'p'; })) {
                ich = 0;
                for (iPass = 0; iPass < 5; iPass++) {
                    app.state.vrgZip[i].txp.rgia[iPass].SetIAction(szPreset[ich++] - 'a');
                    app.state.vrgZip[i].txp.rgia[iPass].SetCQuan(szPreset[ich++] - 'a');
                    app.state.vrgZip[i].txp.rgia[iPass].SetCQuan((static_cast<std::uint32_t>((szPreset[ich++] - 'a')) & 0xfff) << 4 |
                                                       (app.state.vrgZip[i].txp.rgia[iPass].cQuan() | app.state.vrgZip[i].txp.rgia[iPass].iAction() << 0xc));
                    app.state.vrgZip[i].txp.rgia[iPass].SetCQuan((static_cast<std::uint32_t>((szPreset[ich++] - 'a')) & 0xff) << 8 |
                                                       (app.state.vrgZip[i].txp.rgia[iPass].cQuan() | app.state.vrgZip[i].txp.rgia[iPass].iAction() << 0xc));
                }
                CchCopyUtf8Prefix(std::span{app.state.vrgZip[i].szName}, szPreset.substr(20));
                app.state.vrgZip[i].fValid = 1;
            }
        }
    }
    std::fill_n(app.state.vrgZipProd, 5, ZIPPRODQ{});
    for (i = 0; i < 5; i++) {
        CchSnprintf(std::span{szEntry}, "%sP%d", szSection, i + 1);
        cch = ProfileTextIO().pfnReadString(szSection, szEntry, "", app.state.szWork, 80, szIniFile);
        szPreset = std::string_view{app.state.szWork, static_cast<std::size_t>(cch)};
        if (szPreset.size() >= 3 && szPreset.size() <= 64) {
            cpq = szPreset[1] - 'a';
            if (cpq >= 0 && cpq <= 12) {
                cchPrefix = static_cast<std::size_t>(cpq * 4 + 2);
                if (cchPrefix <= szPreset.size() && std::all_of(szPreset.begin(), szPreset.begin() + cchPrefix, [](char ch) { return ch >= 'a' && ch <= 'p'; }) &&
                    szPreset.size() - cchPrefix <= 12) {
                    CchCopyUtf8Prefix(std::span{app.state.vrgZipProd[i].szName}, szPreset.substr(cchPrefix));
                    app.state.vrgZipProd[i].fNoResearch = szPreset[0] != 'a';
                    app.state.vrgZipProd[i].fValid = 1;
                    app.state.vrgZipProd[i].cpq = cpq;
                    ich = 2;
                    for (iPass = 0; iPass < cpq; iPass++) {
                        app.state.vrgZipProd[i].rgpq[iPass].w = szPreset[ich++] - 'a';
                        app.state.vrgZipProd[i].rgpq[iPass].w |= (szPreset[ich++] - 'a') * 0x10;
                        app.state.vrgZipProd[i].rgpq[iPass].w |= (szPreset[ich++] - 'a') * 0x100;
                        app.state.vrgZipProd[i].rgpq[iPass].w |= (szPreset[ich++] - 'a') * 0x1000;
                        if (app.state.vrgZipProd[i].rgpq[iPass].cQuan() > 1020) {
                            app.state.vrgZipProd[i].rgpq[iPass].SetCQuan(1);
                        }
                        if (app.state.vrgZipProd[i].rgpq[iPass].mdIdle() >= mdIdleFactory) {
                            app.state.vrgZipProd[i].rgpq[iPass].SetMdIdle(iobjMine);
                        }
                    }
                }
            }
        }
    }
    TextIO().pfnCchGetString(app, idsDefault, std::span{app.state.vrgZipProd[0].szName});
    app.state.vrgZipProd[0].fValid = 1;
    return;
}

void WriteIniPresets(APPSESSION &app, const char *szIniFile) {
    std::int16_t i, iPass;
    std::size_t ich;
    char    szSection[16], szEntry[16];

    if (app.state.gd.fChgZipOrd) {
        TextIO().pfnCchGetString(app, idsZiporders, std::span{szSection});
        for (i = 0; i < 4; i++) {
            CchSnprintf(std::span{szEntry}, "%s%d", szSection, i + 1);
            if (app.state.vrgZip[i].fValid) {
                ich = 0;
                for (iPass = 0; iPass < 5; iPass++) {
                    app.state.szWork[ich++] = app.state.vrgZip[i].txp.rgia[iPass].iAction() + 'a';
                    app.state.szWork[ich++] = (app.state.vrgZip[i].txp.rgia[iPass].cQuan() & 0xf) + 'a';
                    app.state.szWork[ich++] = (app.state.vrgZip[i].txp.rgia[iPass].cQuan() >> 4 & 0xf) + 'a';
                    app.state.szWork[ich++] = (app.state.vrgZip[i].txp.rgia[iPass].cQuan() >> 8 & 0xf) + 'a';
                }
                CchCopyUtf8(std::span{app.state.szWork}.subspan(ich), app.state.vrgZip[i].szName);
            } else {
                app.state.szWork[0] = 0;
            }
            ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        }
    }
    if (app.state.gd.fChgZipProd) {
        for (i = 0; i < 5; i++) {
            TextIO().pfnCchGetString(app, idsZiporders, std::span{szSection});
            CchSnprintf(std::span{szEntry}, "%sP%d", szSection, i + 1);
            if (app.state.vrgZipProd[i].fValid) {
                ich = 0;
                app.state.szWork[ich++] = app.state.vrgZipProd[i].fNoResearch + 'a';
                app.state.szWork[ich++] = app.state.vrgZipProd[i].cpq + 'a';
                for (iPass = 0; iPass < app.state.vrgZipProd[i].cpq; iPass++) {
                    app.state.szWork[ich++] = (app.state.vrgZipProd[i].rgpq[iPass].w & 0xf) + 'a';
                    app.state.szWork[ich++] = (app.state.vrgZipProd[i].rgpq[iPass].w >> 4 & 0xf) + 'a';
                    app.state.szWork[ich++] = (app.state.vrgZipProd[i].rgpq[iPass].w >> 8 & 0xf) + 'a';
                    app.state.szWork[ich++] = (app.state.vrgZipProd[i].rgpq[iPass].w >> 0xc & 0xf) + 'a';
                }
                CchCopyUtf8(std::span{app.state.szWork}.subspan(ich), app.state.vrgZipProd[i].szName);
            } else {
                app.state.szWork[0] = 0;
            }
            ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        }
    }
    return;
}

void ReadIniSelection(APPSESSION &app, std::string_view szSelection) {
    std::int32_t iObjSel = 0;
    std::from_chars_result result;

    if (szSelection.size() < 3) {
        app.state.ini.grobjSel = 0;
    } else {
        switch (szSelection[0]) {
        case 'N':
        default:
            app.state.ini.grobjSel = 0;
            break;
        case 'P':
            app.state.ini.grobjSel = 1;
            break;
        case 'S':
            app.state.ini.grobjSel = 2;
            break;
        case 'E':
            app.state.ini.grobjSel = 4;
        }
        if (szSelection[1] >= 'B' && szSelection[1] <= 'Q') {
            app.state.ini.idPlayer = szSelection[1] - 'B';
        } else {
            app.state.ini.grobjSel = 0;
        }
        if (app.state.ini.grobjSel != 0) {
            result = std::from_chars(szSelection.data() + 2, szSelection.data() + szSelection.size(), iObjSel);
            app.state.ini.iObjSel = result.ec == std::errc{} ? iObjSel : 0;
        }
    }
}

void ReadIniIdentity(APPSESSION &app, std::string_view szIdentity) {
    std::size_t i;

    app.state.ini.lid = 0;
    for (i = 0; i < szIdentity.size(); i++) {
        app.state.ini.lid = static_cast<std::int32_t>((app.state.ini.lid * 16));
        if (szIdentity[i] >= '0' && szIdentity[i] <= '9') {
            app.state.ini.lid += static_cast<std::int16_t>((szIdentity[i] - '0'));
        } else if (szIdentity[i] >= 'a' && szIdentity[i] <= 'f') {
            app.state.ini.lid += static_cast<std::int16_t>((szIdentity[i] - 'W'));
        }
    }
}

void ReadIniRecent(APPSESSION &app, const char *szSection, const char *szEntry, const char *szIniFile, std::string_view szRecentPath) {
    std::int16_t i, iPass;
    std::int16_t cch;
    std::string szEntryCurrent{szEntry};

    if (szRecentPath.size() > 3) {
        app.state.ini.fStartupFile = 1;
        CchCopyUtf8(std::span{app.state.szBase}, szRecentPath);
    } else {
        app.state.ini.fStartupFile = 0;
    }
    if (!app.lpui->vrgszMRU) {
        app.lpui->vrgszMRU = LpAlloc<char>(app, 2304, htPerm);
    }
    for (i = 0; i < 9; i++) {
        szEntryCurrent.back() = static_cast<char>(i + '1');
        cch = ProfileTextIO().pfnReadString(szSection, szEntryCurrent.c_str(), ".", app.lpui->vrgszMRU + 256 * i, 256, szIniFile);
        if (cch < 4) {
            app.lpui->vrgszMRU[i * 256] = 0;
        }
    }
    iPass = 0;
    for (i = 0; i < 9; i++) {
        if (app.lpui->vrgszMRU[i * 256] != 0) {
            if (i != iPass) {
                CchCopyUtf8(std::span{app.lpui->vrgszMRU, std::size_t{2304}}.subspan(256 * iPass, 256), app.lpui->vrgszMRU + 256 * i);
                app.lpui->vrgszMRU[i * 256] = 0;
            }
            iPass++;
        }
    }
}
