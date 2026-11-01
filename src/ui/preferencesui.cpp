#include "preferencesui.h"
#include "appstate.h"
#include "uistate.h"
#include "profile_text_io.h"
#include "text_io.h"
#include "win.h"

void ReadIniPresets(APPSESSION &app, char *szIniFile) {
    int16_t i, iPass, cch, cpq;
    char    szSection[16], szEntry[16], *psz;

    TextIO().pfnCchGetString(app, idsZiporders, szSection);
    memset(app.state.vrgZip, 0, 96);
    for (i = 0; i < 4; i++) {
        strcpy(szEntry, szSection);
        psz = &szEntry[strlen(szEntry)];
        *psz = i + '1';
        psz[1] = 0;
        ProfileTextIO().pfnReadString(szSection, szEntry, "", app.state.szWork, 80, szIniFile);
        cch = strlen(app.state.szWork);
        if (cch >= 20 && cch <= 32) {
            psz = app.state.szWork;
            iPass = 0;
            for (; iPass < 20 && *psz >= 'a' && *psz <= 'p'; psz++) {
                iPass++;
            }
            if (iPass >= 20) {
                psz = app.state.szWork;
                for (iPass = 0; iPass < 5; iPass++) {
                    app.state.vrgZip[i].txp.rgia[iPass].SetIAction(*psz - 'a');
                    psz++;
                    app.state.vrgZip[i].txp.rgia[iPass].SetCQuan(*psz - 'a');
                    psz++;
                    app.state.vrgZip[i].txp.rgia[iPass].SetCQuan(((uint32_t)(*psz - 'a') & 0xfff) << 4 |
                                                       (app.state.vrgZip[i].txp.rgia[iPass].cQuan() | app.state.vrgZip[i].txp.rgia[iPass].iAction() << 0xc));
                    psz++;
                    app.state.vrgZip[i].txp.rgia[iPass].SetCQuan(((uint32_t)(*psz - 'a') & 0xff) << 8 |
                                                       (app.state.vrgZip[i].txp.rgia[iPass].cQuan() | app.state.vrgZip[i].txp.rgia[iPass].iAction() << 0xc));
                    psz++;
                }
                strcpy(app.state.vrgZip[i].szName, psz);
                app.state.vrgZip[i].fValid = 1;
            }
        }
    }
    memset(app.state.vrgZipProd, 0, 200);
    for (i = 0; i < 5; i++) {
        strcpy(szEntry, szSection);
        psz = &szEntry[strlen(szEntry)];
        *psz++ = 'P';
        *psz = i + '1';
        psz[1] = 0;
        ProfileTextIO().pfnReadString(szSection, szEntry, "", app.state.szWork, 80, szIniFile);
        cch = strlen(app.state.szWork);
        if (cch >= 3 && cch <= 64) {
            psz = app.state.szWork;
            cpq = psz[1] - 'a';
            if (cpq >= 0 && cpq <= 12) {
                iPass = 0;
                for (; iPass < cpq * 4 + 2 && *psz >= 'a' && *psz <= 'p'; psz++) {
                    iPass++;
                }
                if (iPass >= cpq * 4 + 2 && strlen(psz) <= 12) {
                    strcpy(app.state.vrgZipProd[i].szName, psz);
                    psz = app.state.szWork;
                    app.state.vrgZipProd[i].fNoResearch = *psz != 'a';
                    app.state.vrgZipProd[i].fValid = 1;
                    app.state.vrgZipProd[i].cpq = cpq;
                    psz += 2;
                    for (iPass = 0; iPass < cpq; iPass++) {
                        app.state.vrgZipProd[i].rgpq[iPass].w = *psz - 'a';
                        psz++;
                        app.state.vrgZipProd[i].rgpq[iPass].w |= (*psz - 'a') * 0x10;
                        psz++;
                        app.state.vrgZipProd[i].rgpq[iPass].w |= (*psz - 'a') * 0x100;
                        psz++;
                        app.state.vrgZipProd[i].rgpq[iPass].w |= (*psz - 'a') * 0x1000;
                        psz++;
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
    TextIO().pfnCchGetString(app, idsDefault, app.state.vrgZipProd[0].szName);
    app.state.vrgZipProd[0].fValid = 1;
    return;
}

void WriteIniPresets(APPSESSION &app, char *szIniFile) {
    int16_t i, iPass;
    char    szSection[16], szEntry[16], *psz;

    if (app.state.gd.fChgZipOrd) {
        TextIO().pfnCchGetString(app, idsZiporders, szSection);
        for (i = 0; i < 4; i++) {
            strcpy(szEntry, szSection);
            psz = &szEntry[strlen(szEntry)];
            *psz = i + '1';
            psz[1] = 0;
            if (app.state.vrgZip[i].fValid) {
                psz = app.state.szWork;
                for (iPass = 0; iPass < 5; iPass++) {
                    *psz++ = app.state.vrgZip[i].txp.rgia[iPass].iAction() + 'a';
                    *psz++ = (app.state.vrgZip[i].txp.rgia[iPass].cQuan() & 0xf) + 'a';
                    *psz++ = (app.state.vrgZip[i].txp.rgia[iPass].cQuan() >> 4 & 0xf) + 'a';
                    *psz++ = (app.state.vrgZip[i].txp.rgia[iPass].cQuan() >> 8 & 0xf) + 'a';
                }
                strcpy(psz, app.state.vrgZip[i].szName);
            } else {
                app.state.szWork[0] = 0;
            }
            ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        }
    }
    if (app.state.gd.fChgZipProd) {
        for (i = 0; i < 5; i++) {
            TextIO().pfnCchGetString(app, idsZiporders, szSection);
            strcpy(szEntry, szSection);
            psz = &szEntry[strlen(szEntry)];
            *psz++ = 'P';
            *psz = i + '1';
            psz[1] = 0;
            if (app.state.vrgZipProd[i].fValid) {
                psz = app.state.szWork;
                *psz++ = app.state.vrgZipProd[i].fNoResearch + 'a';
                *psz++ = app.state.vrgZipProd[i].cpq + 'a';
                for (iPass = 0; iPass < app.state.vrgZipProd[i].cpq; iPass++) {
                    *psz++ = (app.state.vrgZipProd[i].rgpq[iPass].w & 0xf) + 'a';
                    *psz++ = (app.state.vrgZipProd[i].rgpq[iPass].w >> 4 & 0xf) + 'a';
                    *psz++ = (app.state.vrgZipProd[i].rgpq[iPass].w >> 8 & 0xf) + 'a';
                    *psz++ = (app.state.vrgZipProd[i].rgpq[iPass].w >> 0xc & 0xf) + 'a';
                }
                strcpy(psz, app.state.vrgZipProd[i].szName);
            } else {
                app.state.szWork[0] = 0;
            }
            ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        }
    }
    return;
}

void ReadIniSelection(APPSESSION &app, const char *pszSelection, int16_t cch) {
    if (cch < 3) {
        app.state.ini.grobjSel = 0;
    } else {
        switch (pszSelection[0]) {
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
        if (pszSelection[1] >= 'B' && pszSelection[1] <= 'Q') {
            app.state.ini.idPlayer = pszSelection[1] - 'B';
        } else {
            app.state.ini.grobjSel = 0;
        }
        if (app.state.ini.grobjSel != 0) {
            app.state.ini.iObjSel = atoi(&pszSelection[2]);
        }
    }
}

void ReadIniIdentity(APPSESSION &app, const char *pszIdentity, int16_t cch) {
    int16_t i;

    app.state.ini.lid = 0;
    for (i = 0; i < cch; i++) {
        app.state.ini.lid = (int32_t)(app.state.ini.lid * 16);
        if (pszIdentity[i] >= '0' && pszIdentity[i] <= '9') {
            app.state.ini.lid += (int16_t)(pszIdentity[i] - '0');
        } else if (pszIdentity[i] >= 'a' && pszIdentity[i] <= 'f') {
            app.state.ini.lid += (int16_t)(pszIdentity[i] - 'W');
        }
    }
}

void ReadIniRecent(APPSESSION &app, char *szSection, char *szEntry, char *szIniFile, int16_t cch) {
    int16_t i, iPass;
    char   *psz;

    if (cch > 3) {
        app.state.ini.fStartupFile = 1;
        strcpy(app.state.szBase, app.state.szWork);
    } else {
        app.state.ini.fStartupFile = 0;
    }
    if (!app.lpui->vrgszMRU) {
        app.lpui->vrgszMRU = LpAlloc<char>(app, 2304, htPerm);
    }
    psz = &szEntry[strlen(szEntry) - 1];
    for (i = 0; i < 9; i++) {
        *psz = i + '1';
        cch = ProfileTextIO().pfnReadString(szSection, szEntry, ".", app.lpui->vrgszMRU + 256 * i, 256, szIniFile);
        if (cch < 4) {
            app.lpui->vrgszMRU[i * 256] = 0;
        }
    }
    iPass = 0;
    for (i = 0; i < 9; i++) {
        if (app.lpui->vrgszMRU[i * 256] != 0) {
            if (i != iPass) {
                strcpy(app.lpui->vrgszMRU + 256 * iPass, app.lpui->vrgszMRU + 256 * i);
                app.lpui->vrgszMRU[i * 256] = 0;
            }
            iPass++;
        }
    }
}
