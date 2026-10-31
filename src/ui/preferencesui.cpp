#include "preferencesui.h"
#include "profile_text_io.h"
#include "text_io.h"
#include "win.h"

void ReadIniPresets(char *szIniFile) {
    int16_t i, iPass, cch, cpq;
    char    szSection[16], szEntry[16], *psz;

    TextIO().pfnCchGetString(idsZiporders, szSection);
    memset(vrgZip, 0, 96);
    for (i = 0; i < 4; i++) {
        strcpy(szEntry, szSection);
        psz = &szEntry[strlen(szEntry)];
        *psz = i + '1';
        psz[1] = 0;
        ProfileTextIO().pfnReadString(szSection, szEntry, "", szWork, 80, szIniFile);
        cch = strlen(szWork);
        if (cch >= 20 && cch <= 32) {
            psz = szWork;
            iPass = 0;
            for (; iPass < 20 && *psz >= 'a' && *psz <= 'p'; psz++) {
                iPass++;
            }
            if (iPass >= 20) {
                psz = szWork;
                for (iPass = 0; iPass < 5; iPass++) {
                    vrgZip[i].txp.rgia[iPass].SetIAction(*psz - 'a');
                    psz++;
                    vrgZip[i].txp.rgia[iPass].SetCQuan(*psz - 'a');
                    psz++;
                    vrgZip[i].txp.rgia[iPass].SetCQuan(((uint32_t)(*psz - 'a') & 0xfff) << 4 |
                                                       (vrgZip[i].txp.rgia[iPass].cQuan() | vrgZip[i].txp.rgia[iPass].iAction() << 0xc));
                    psz++;
                    vrgZip[i].txp.rgia[iPass].SetCQuan(((uint32_t)(*psz - 'a') & 0xff) << 8 |
                                                       (vrgZip[i].txp.rgia[iPass].cQuan() | vrgZip[i].txp.rgia[iPass].iAction() << 0xc));
                    psz++;
                }
                strcpy(vrgZip[i].szName, psz);
                vrgZip[i].fValid = 1;
            }
        }
    }
    memset(vrgZipProd, 0, 200);
    for (i = 0; i < 5; i++) {
        strcpy(szEntry, szSection);
        psz = &szEntry[strlen(szEntry)];
        *psz++ = 'P';
        *psz = i + '1';
        psz[1] = 0;
        ProfileTextIO().pfnReadString(szSection, szEntry, "", szWork, 80, szIniFile);
        cch = strlen(szWork);
        if (cch >= 3 && cch <= 64) {
            psz = szWork;
            cpq = psz[1] - 'a';
            if (cpq >= 0 && cpq <= 12) {
                iPass = 0;
                for (; iPass < cpq * 4 + 2 && *psz >= 'a' && *psz <= 'p'; psz++) {
                    iPass++;
                }
                if (iPass >= cpq * 4 + 2 && strlen(psz) <= 12) {
                    strcpy(vrgZipProd[i].szName, psz);
                    psz = szWork;
                    vrgZipProd[i].fNoResearch = *psz != 'a';
                    vrgZipProd[i].fValid = 1;
                    vrgZipProd[i].cpq = cpq;
                    psz += 2;
                    for (iPass = 0; iPass < cpq; iPass++) {
                        vrgZipProd[i].rgpq[iPass].w = *psz - 'a';
                        psz++;
                        vrgZipProd[i].rgpq[iPass].w |= (*psz - 'a') * 0x10;
                        psz++;
                        vrgZipProd[i].rgpq[iPass].w |= (*psz - 'a') * 0x100;
                        psz++;
                        vrgZipProd[i].rgpq[iPass].w |= (*psz - 'a') * 0x1000;
                        psz++;
                        if (vrgZipProd[i].rgpq[iPass].cQuan() > 1020) {
                            vrgZipProd[i].rgpq[iPass].SetCQuan(1);
                        }
                        if (vrgZipProd[i].rgpq[iPass].mdIdle() >= mdIdleFactory) {
                            vrgZipProd[i].rgpq[iPass].SetMdIdle(iobjMine);
                        }
                    }
                }
            }
        }
    }
    TextIO().pfnCchGetString(idsDefault, vrgZipProd[0].szName);
    vrgZipProd[0].fValid = 1;
    return;
}

void WriteIniPresets(char *szIniFile) {
    int16_t i, iPass;
    char    szSection[16], szEntry[16], *psz;

    if (gd.fChgZipOrd) {
        TextIO().pfnCchGetString(idsZiporders, szSection);
        for (i = 0; i < 4; i++) {
            strcpy(szEntry, szSection);
            psz = &szEntry[strlen(szEntry)];
            *psz = i + '1';
            psz[1] = 0;
            if (vrgZip[i].fValid) {
                psz = szWork;
                for (iPass = 0; iPass < 5; iPass++) {
                    *psz++ = vrgZip[i].txp.rgia[iPass].iAction() + 'a';
                    *psz++ = (vrgZip[i].txp.rgia[iPass].cQuan() & 0xf) + 'a';
                    *psz++ = (vrgZip[i].txp.rgia[iPass].cQuan() >> 4 & 0xf) + 'a';
                    *psz++ = (vrgZip[i].txp.rgia[iPass].cQuan() >> 8 & 0xf) + 'a';
                }
                strcpy(psz, vrgZip[i].szName);
            } else {
                szWork[0] = 0;
            }
            ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
        }
    }
    if (gd.fChgZipProd) {
        for (i = 0; i < 5; i++) {
            TextIO().pfnCchGetString(idsZiporders, szSection);
            strcpy(szEntry, szSection);
            psz = &szEntry[strlen(szEntry)];
            *psz++ = 'P';
            *psz = i + '1';
            psz[1] = 0;
            if (vrgZipProd[i].fValid) {
                psz = szWork;
                *psz++ = vrgZipProd[i].fNoResearch + 'a';
                *psz++ = vrgZipProd[i].cpq + 'a';
                for (iPass = 0; iPass < vrgZipProd[i].cpq; iPass++) {
                    *psz++ = (vrgZipProd[i].rgpq[iPass].w & 0xf) + 'a';
                    *psz++ = (vrgZipProd[i].rgpq[iPass].w >> 4 & 0xf) + 'a';
                    *psz++ = (vrgZipProd[i].rgpq[iPass].w >> 8 & 0xf) + 'a';
                    *psz++ = (vrgZipProd[i].rgpq[iPass].w >> 0xc & 0xf) + 'a';
                }
                strcpy(psz, vrgZipProd[i].szName);
            } else {
                szWork[0] = 0;
            }
            ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
        }
    }
    return;
}

void ReadIniSelection(const char *pszSelection, int16_t cch) {
    if (cch < 3) {
        ini.grobjSel = 0;
    } else {
        switch (pszSelection[0]) {
        case 'N':
        default:
            ini.grobjSel = 0;
            break;
        case 'P':
            ini.grobjSel = 1;
            break;
        case 'S':
            ini.grobjSel = 2;
            break;
        case 'E':
            ini.grobjSel = 4;
        }
        if (pszSelection[1] >= 'B' && pszSelection[1] <= 'Q') {
            ini.idPlayer = pszSelection[1] - 'B';
        } else {
            ini.grobjSel = 0;
        }
        if (ini.grobjSel != 0) {
            ini.iObjSel = atoi(&pszSelection[2]);
        }
    }
}

void ReadIniIdentity(const char *pszIdentity, int16_t cch) {
    int16_t i;

    ini.lid = 0;
    for (i = 0; i < cch; i++) {
        ini.lid = (int32_t)(ini.lid * 16);
        if (pszIdentity[i] >= '0' && pszIdentity[i] <= '9') {
            ini.lid += (int16_t)(pszIdentity[i] - '0');
        } else if (pszIdentity[i] >= 'a' && pszIdentity[i] <= 'f') {
            ini.lid += (int16_t)(pszIdentity[i] - 'W');
        }
    }
}

void ReadIniRecent(char *szSection, char *szEntry, char *szIniFile, int16_t cch) {
    int16_t i, iPass;
    char   *psz;

    if (cch > 3) {
        ini.fStartupFile = 1;
        strcpy(szBase, szWork);
    } else {
        ini.fStartupFile = 0;
    }
    if (!vrgszMRU) {
        vrgszMRU = LpAlloc<char>(2304, htPerm);
    }
    psz = &szEntry[strlen(szEntry) - 1];
    for (i = 0; i < 9; i++) {
        *psz = i + '1';
        cch = ProfileTextIO().pfnReadString(szSection, szEntry, ".", vrgszMRU + 256 * i, 256, szIniFile);
        if (cch < 4) {
            vrgszMRU[i * 256] = 0;
        }
    }
    iPass = 0;
    for (i = 0; i < 9; i++) {
        if (vrgszMRU[i * 256] != 0) {
            if (i != iPass) {
                strcpy(vrgszMRU + 256 * iPass, vrgszMRU + 256 * i);
                vrgszMRU[i * 256] = 0;
            }
            iPass++;
        }
    }
}
