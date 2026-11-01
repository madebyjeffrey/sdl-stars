#include "common.h"
#include "appstate.h"
#include "text_io.h"
#include "player_choice_io.h"
#include "record_io.h"

int16_t rgPrimes[128] = {3,   5,   7,   11,  13,  17,  19,  23,  29,  31,  37,  41,  43,  47,  53,  59,  61,  67,  71,  73,  79,  83,  89,  97,  101, 103,
                         107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241,
                         251, 257, 263, 279, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401,
                         409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563, 569, 571,
                         577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701, 709, 719, 727};

void PushRandom(APPSESSION &app, int32_t lNew1, int32_t lNew2) {
    app.state.rglRandStack[app.state.cRandStack][0] = app.state.lRandSeed1;
    app.state.rglRandStack[app.state.cRandStack][1] = app.state.lRandSeed2;
    app.state.cRandStack++;
    app.state.lRandSeed1 = lNew1;
    app.state.lRandSeed2 = lNew2;
    return;
}

void PopRandom(APPSESSION &app) {
    app.state.cRandStack--;
    app.state.lRandSeed1 = app.state.rglRandStack[app.state.cRandStack][0];
    app.state.lRandSeed2 = app.state.rglRandStack[app.state.cRandStack][1];
    return;
}

void Randomize(APPSESSION &app, uint32_t dw) {
    int16_t a;
    int16_t b;

    a = LOWORD(dw) & 0x3f;
    b = LOWORD(static_cast<uint32_t>((dw >> 6))) & 0x3f;
    if (a == b) {
        b = (b + 1) & 0x3f;
    }
    app.state.lRandSeed1 = rgPrimes[a];
    app.state.lRandSeed2 = rgPrimes[b];
    return;
}

void Randomize2(APPSESSION &app, uint32_t dw) {
    int16_t a;
    int16_t b;

    a = LOWORD(dw) & 0x7f;
    b = LOWORD(static_cast<uint32_t>((dw >> 7))) & 0x7f;
    a ^= 0x35;
    b ^= 0x5c;
    if (a == b) {
        b = (b + 1) & 0x7f;
    }
    app.state.lRandSeed1 = rgPrimes[a];
    app.state.lRandSeed2 = rgPrimes[b];
    return;
}

int16_t Random(APPSESSION &app, int16_t c) {
    int32_t z;
    int32_t s1;
    int32_t k;
    int32_t s2;

    s1 = app.state.lRandSeed1;
    s2 = app.state.lRandSeed2;
    k = static_cast<int32_t>((s1 / 53668));
    s1 = static_cast<uint32_t>(((s1 - static_cast<uint32_t>((k * 53668))) * 0x9c4e)) - static_cast<uint32_t>((k * 12211));
    if (s1 < 0) {
        s1 += 2147483563;
    }
    k = static_cast<int32_t>((s2 / 52774));
    s2 = static_cast<uint32_t>(((s2 - static_cast<uint32_t>((k * 52774))) * 0x9ef4)) - static_cast<uint32_t>((k * 3791));
    if (s2 < 0) {
        s2 += 2147483399;
    }
    z = s1 - s2;
    if (z < 1) {
        z += 2147483562;
    }
    app.state.lRandSeed1 = s1;
    app.state.lRandSeed2 = s2;
    if (c <= 0) {
        return 0;
    }
    return LOWORD(static_cast<uint32_t>((static_cast<uint32_t>(z) % c)));
}

void GetFileSeeds(APPSESSION &app, int32_t *pl1, int32_t *pl2) {
    *pl1 = app.state.lFileSeed1;
    *pl2 = app.state.lFileSeed2;
    return;
}

void SetFileSeeds(APPSESSION &app, int32_t l1, int32_t l2) {
    app.state.lFileSeed1 = l1;
    app.state.lFileSeed2 = l2;
    return;
}

void SetFileXorStream(APPSESSION &app, int32_t lid, int16_t lSalt, int16_t turn, int16_t iPlayer, int16_t fCrippled) {
    int16_t a;
    int16_t b;

    a = lSalt & 0x1f;
    b = lSalt >> 5 & 0x1f;
    if (lSalt & 0x400) {
        a += 32;
    } else {
        b += 32;
    }
    app.state.lFileSeed1 = rgPrimes[a];
    app.state.lFileSeed2 = rgPrimes[b];
    a = ((LOWORD(lid) & 3) + 1) * ((turn & 3) + 1) * ((iPlayer & 3) + 1) + fCrippled;
    while (a-- > 0) {
        LGetNextFileXor(app);
    }
    return;
}

int32_t LGetNextFileXor(APPSESSION &app) {
    int32_t s1;
    int32_t k;
    int32_t s2;

    s1 = app.state.lFileSeed1;
    s2 = app.state.lFileSeed2;
    k = static_cast<int32_t>((s1 / 53668));
    s1 = static_cast<uint32_t>(((s1 - static_cast<uint32_t>((k * 53668))) * 0x9c4e)) - static_cast<uint32_t>((k * 12211));
    if (s1 < 0) {
        s1 += 2147483563;
    }
    k = static_cast<int32_t>((s2 / 52774));
    s2 = static_cast<uint32_t>(((s2 - static_cast<uint32_t>((k * 52774))) * 0x9ef4)) - static_cast<uint32_t>((k * 3791));
    if (s2 < 0) {
        s2 += 2147483399;
    }
    app.state.lFileSeed1 = s1;
    app.state.lFileSeed2 = s2;
    return s1 - s2;
}

void XorFileBuf(APPSESSION &app, char *rgb, int16_t cb) {
    char    *plMac;
    char    *pl;
    int32_t  lPrev;
    uint32_t ul;
    char    *pch;

    lPrev = 0;
    pl = rgb;
    plMac = pl + (cb >> 2) * 4;
    for (; pl < plMac; pl += 4) {
        lPrev = LGetNextFileXor(app);
        NATIVEBYTEREADER(std::as_bytes(std::span(pl, 4))).FReadU32Le(ul);
        ul ^= static_cast<uint32_t>(lPrev);
        NATIVEBYTEWRITER(std::as_writable_bytes(std::span(pl, 4))).FWriteU32Le(ul);
    }
    cb &= 3;
    if (cb > 0) {
        pch = static_cast<char *>(pl);
        lPrev = LGetNextFileXor(app);
        while (cb-- != 0) {
            *pch ^= static_cast<int16_t>(static_cast<int8_t>(LOBYTE(LOWORD(lPrev) & 0xff)));
            lPrev = static_cast<int32_t>((lPrev >> 8));
            pch++;
        }
    }
    return;
}

int ICompLong(APPSESSION &, int32_t *pl1, int32_t *pl2) { return static_cast<int16_t>((*pl1 - *pl2)); }

char *PszGetCompressedPlanet(APPSESSION &app, int16_t id) {
    int16_t  fCap;
    int16_t  iOffset;
    int16_t  fHigh;
    int16_t  iChunk;
    int16_t  i;
    int16_t  iBuild;
    int16_t  iNibble;
    uint8_t *pchLen;
    char    *pszOut;
    uint8_t *pch;
    int16_t  iLen;

    iNibble = 0;
    if (id >= cPlanetName) {
        id %= cPlanetName;
    }
    if (id == app.state.iLastGet) {
        return app.state.szLastGet;
    }
    iChunk = id >> 6;
    iOffset = id & 0x3f;
    pch = reinterpret_cast<uint8_t *>(&aPNCmpr[aiPNChunkOffset[iChunk]]);
    pchLen = &acPN[iChunk * 64];
    i = 0;
    while (i < iOffset) {
        iNibble += *pchLen;
        i++;
        pchLen++;
    }
    pch += iNibble >> 1;
    iLen = *pchLen;
    fHigh = (iNibble & 1) == 0;
    pszOut = app.state.szLastGet;
    iBuild = 0;
    fCap = 1;
    while (iLen-- != 0) {
        if (fHigh) {
            i = *pch >> 4;
        } else {
            i = *pch++ & 0xf;
        }
        fHigh = fHigh == 0;
        iBuild += i;
        if (i != 15) {
            *pszOut = rgPNLookupTable[iBuild];
            if (fCap && *pszOut >= 'a' && *pszOut <= 'z') {
                *pszOut -= ' ';
            }
            if (*pszOut == ' ' || *pszOut == '-') {
                fCap = 1;
            } else {
                fCap = 0;
            }
            pszOut++;
            iBuild = 0;
        }
    }
    *pszOut = 0;
    return app.state.szLastGet;
}

void OutputFileString(const char *szFile, const char *sz) {
    int16_t  fMissing;
    uint16_t w;
    int16_t  hf;

    w = 2;
    if (!FFileExists(szFile)) {
        w |= 0x1000;
    }
    hf = HfOpenFile(szFile, w, &fMissing);
    if (hf != -1) {
        LSeekFile(hf, 0, 2);
        CbWriteFile(hf, sz, strlen(sz));
        CloseFile(hf);
    }
    return;
}

void StarsCopyFile(APPSESSION &app, const char *szSrc, const char *szDst) {
    char     rgb[2048];
    int16_t  fMissing;
    int16_t  hfDst;
    int32_t  cb;

    hfDst = -1;
    StarsRestore<int16_t> restoreSilent(app.state.fFileErrSilent);
    StarsFileGuard closeDestination(hfDst);
    StarsRecoveryScope scopeRecovery;
    app.state.fFileErrSilent = 1;
    try {
        RecordIO().pfnStreamOpen(app, szSrc, mdRead);
        hfDst = HfOpenFile(szDst, 4114, &fMissing);
        if (hfDst == -1)
            goto LStreamError;
        for (cb = CbFileSize(app.state.hf); cb > 2048; cb -= 2048) {
            RgFromStream(app, rgb, 0x800);
            if (CbWriteFile(hfDst, rgb, 0x800) != 0x800)
                goto LStreamError;
        }
        if (cb != 0) {
            RgFromStream(app, rgb, LOWORD(cb));
            CbWriteFile(hfDst, rgb, LOWORD(cb));
        }
    } catch (const StarsError &) {
    }
LStreamError:
    RecordIO().pfnStreamClose(app);
    return;
}

int16_t AlertSz(APPSESSION &app, const char *sz, int16_t mbType) {
    char szT[256];

    if (app.state.ini.fValidate || (app.state.ini.fLogging && app.state.ini.fGen)) {
        CchSprintf(szT, "Error: %s", sz);
        OutputSz(app, !app.state.ini.fValidate ? 6 : 7, szT);
        return win32::IDYES;
    }
    return IdAlertBox(sz, mbType);
}

int16_t CchGetString(APPSESSION &app, StringId ids, char *psz) {
    char *pszT;
    char *pszTT;

    pszTT = psz;
    pszT = TextIO().pfnCompressed(app, ids);
    while (*pszT != 0) {
        *psz++ = *pszT++;
    }
    *psz = 0;
    return psz - pszTT;
}

char *PszFromInt(APPSESSION &app, int16_t i, int16_t *pcch) {
    int16_t cch;

    cch = CchSprintf(app.state.szFormatNumber, app.state.PCTD, i);
    if (pcch) {
        *pcch = cch;
    }
    return app.state.szFormatNumber;
}

char *PszFromLong(APPSESSION &app, int32_t l, int16_t *pcch) {
    int16_t cch;

    cch = CchSprintf(app.state.szFormatNumber, app.state.PCTLD, l);
    /* The original tested *pcch, reading through the report dumps' NULL. */
    if (pcch) {
        *pcch = cch;
    }
    return app.state.szFormatNumber;
}

char *PszFromLongK(APPSESSION &app, int32_t l, int16_t *pcch) {
    int16_t fExtraLarge;
    int16_t fLarge;
    char   *psz;

    fLarge = l >= 10000;
    fExtraLarge = l >= 1000000;
    if (fExtraLarge) {
        l = static_cast<int32_t>(((l + 500000) / 0xf4240));
        if (l > 999) {
            l = 999;
        }
    } else if (fLarge) {
        l = static_cast<int32_t>(((l + 500) / 1000));
        if (l > 999) {
            l = 999;
        }
    }
    psz = PszFromInt(app, LOWORD(l), pcch);
    if (fExtraLarge) {
        psz[(*pcch)++] = 'M';
    } else if (fLarge) {
        psz[(*pcch)++] = 'k';
    }
    return psz;
}

int16_t CommaFormatLong(APPSESSION &app, char *psz, int32_t l) {
    char    rgch[15];
    int16_t c;
    int16_t cSkip;
    char   *pchOut;
    char   *pch;

    c = CchSprintf(rgch, app.state.PCTLD, l);
    pch = rgch;
    pchOut = psz;
    cSkip = c % 3;
    if (cSkip == 0) {
        cSkip = 3;
    } else if (cSkip == 1 && l < 0) {
        cSkip = 4;
    }
    while (cSkip-- > 0) {
        *pchOut++ = *pch++;
    }
    while (*pch != 0) {
        cSkip = 3;
        *pchOut++ = ',';
        while (cSkip-- > 0) {
            *pchOut++ = *pch++;
        }
    }
    *pchOut = 0;
    return pchOut - psz;
}

void BoundPoints(RECT *prc, POINT16 *rgpt, int16_t cpt) {
    int16_t ipt;
    int16_t xMax;
    int16_t yMax;
    int16_t xMin;
    int16_t yMin;

    xMax = rgpt->x;
    xMin = rgpt->x;
    yMax = rgpt->y;
    yMin = rgpt->y;
    for (ipt = 1; ipt < cpt; ipt++) {
        xMin = xMin >= rgpt[ipt].x ? rgpt[ipt].x : xMin;
        xMax = xMax <= rgpt[ipt].x ? rgpt[ipt].x : xMax;
        yMin = yMin >= rgpt[ipt].y ? rgpt[ipt].y : yMin;
        yMax = yMax <= rgpt[ipt].y ? rgpt[ipt].y : yMax;
    }
    prc->top = yMin - 1;
    prc->bottom = yMax + 1;
    prc->left = xMin - 1;
    prc->right = xMax + 1;
    return;
}

int16_t FCompressUserString(APPSESSION &app, const char *szIn, char *szOut, int16_t *pcOut) {
    int16_t fHalf;
    char    szWork[1024];
    int16_t iNyb;
    char   *pchOut;
    int16_t cNyb;

    fHalf = 0;
    pchOut = szWork;
    for (; *szIn != 0; szIn++) {
        iNyb = NybbleFromCh(app, *szIn);
        if (iNyb < 11) {
            cNyb = 1;
        } else if ((iNyb & 0xf) != 0xf) {
            cNyb = 2;
        } else {
            cNyb = 3;
        }
        while (cNyb-- != 0) {
            if (!fHalf) {
                *pchOut = (iNyb & 0xf) * 0x10;
                fHalf = 1;
            } else {
                *pchOut |= iNyb & 0xf;
                pchOut++;
                fHalf = 0;
                if (pchOut - szWork >= 0x400) {
                    return 0;
                }
            }
            iNyb >>= 4;
        }
    }
    if (fHalf) {
        *pchOut |= 0xf;
        pchOut++;
    }
    if (pchOut - szWork > *pcOut) {
        return 0;
    }
    *pcOut = pchOut - szWork;
    memcpy(szOut, szWork, *pcOut);
    return 1;
}

static int16_t FDecompressUserStringInternal(APPSESSION &app, const char *szIn, int16_t cIn, char *szOut, int16_t *pcOut, int16_t fUtf8) {
    int16_t fHalf;
    char    szWork[1024];
    int16_t iNyb;
    char   *pchOut;

    fHalf = 0;
    pchOut = szWork;
    if (cIn < 0 || *pcOut < 0 || *pcOut >= static_cast<int16_t>(sizeof(szWork))) goto invalid;
    while (cIn > 0) {
        if (fHalf) {
            iNyb = *szIn & 0xf;
            cIn--;
            szIn++;
            if (iNyb == 15 && cIn == 0)
                break;
        } else {
            iNyb = *szIn >> 4 & 0xf;
        }
        fHalf = fHalf == 0;
        if (iNyb >= 11) {
            if (cIn == 0) goto invalid;
            if (fHalf) {
                iNyb |= (*szIn & 0xf) << 4;
                cIn--;
                szIn++;
            } else {
                iNyb |= *szIn & 0xf0;
            }
            fHalf = fHalf == 0;
            if ((iNyb & 0xf) == 0xf) {
                if (cIn == 0) goto invalid;
                if (fHalf) {
                    iNyb |= (*szIn & 0xf) << 8;
                    cIn--;
                    szIn++;
                } else {
                    iNyb |= (*szIn & 0xf0) << 4;
                }
                fHalf = fHalf == 0;
            }
        }
        *pchOut = ChFromNybble(app, iNyb);
        pchOut++;
        if (pchOut - szWork > *pcOut) {
            goto invalid;
        }
    }
    *pchOut = 0;
    if (memchr(szWork, 0, static_cast<size_t>((pchOut - szWork))) != nullptr) goto invalid;
    if (fUtf8 && !FUtf8Valid(szWork, static_cast<size_t>((pchOut - szWork)))) goto invalid;
    strcpy(szOut, szWork);
    return 1;
invalid:
    szOut[0] = 0;
    return 0;
}

int16_t FDecompressUserString(APPSESSION &app, const char *szIn, int16_t cIn, char *szOut, int16_t *pcOut) {
    return FDecompressUserStringInternal(app, szIn, cIn, szOut, pcOut, 1);
}

int16_t FDecompressUserStringBytes(APPSESSION &app, const char *szIn, int16_t cIn, char *szOut, int16_t *pcOut) {
    return FDecompressUserStringInternal(app, szIn, cIn, szOut, pcOut, 0);
}

int16_t NybbleFromCh(APPSESSION &app, uint8_t ch) {
    char *pch;

    if (ch >= 97 && ch <= 122) {
        return app.state.rgcompstrlower[ch - 97];
    }
    if (ch == 32) {
        return 0;
    }
    if (ch >= 65 && ch <= 80) {
        return (ch - 0x41) << 4 | 0xb;
    }
    if (ch >= 81 && ch <= 90) {
        return (ch - 0x51) << 4 | 0xc;
    }
    if (ch >= 48 && ch <= 53) {
        return (ch - 0x26) << 4 | 0xc;
    }
    if (ch >= 54 && ch <= 57) {
        return (ch - 0x36) << 4 | 0xd;
    }
    pch = strchr(app.state.rgchcomp, ch);
    if (pch) {
        return (pch - app.state.rgchcomp + 4) << 4 | 0xe;
    }
    return ch << 4 | 0xf;
}

char ChFromNybble(APPSESSION &app, int16_t nyb) {
    int16_t iPage;
    int16_t iVal;

    if (nyb < 11) {
        return app.state.rgchcompstrlower[nyb];
    }
    iPage = nyb & 0xf;
    if (iPage == 15) {
        return nyb >> 4;
    }
    iVal = (iPage - 0xb) * 0x10 + (nyb >> 4);
    if (iVal < 26) {
        return iVal + 65;
    }
    if (iVal < 36) {
        return iVal + 22;
    }
    if (iVal >= 52) {
        return app.state.rgchcomp[iVal - 52];
    }
    return app.state.rgchcompstrlower[iVal - 25];
}

int16_t FIntersectCircleLine(POINT16 ptL1, POINT16 ptL2, POINT16 ptC, int32_t r2, int16_t dMax, int16_t *pdStart, int16_t *pdEnd) {
    int32_t dyT;
    int32_t dxdy;
    int16_t dCtr;
    int32_t dy2;
    int32_t dy;
    int32_t yI;
    int32_t dyI;
    int32_t dxT;
    int32_t lT;
    int32_t dx2;
    int16_t dOff;
    int32_t dx;
    int32_t dxI;
    int32_t xI;
    int32_t r2I;

    dx = static_cast<int16_t>((ptL2.x - ptL1.x));
    dy = static_cast<int16_t>((ptL2.y - ptL1.y));
    dxdy = static_cast<uint32_t>((dx * dy));
    dx2 = static_cast<uint32_t>((dx * dx));
    dy2 = static_cast<uint32_t>((dy * dy));
    if (dxdy > 500000 || dx2 > 500000 || dy2 > 500000) {
        dxI = static_cast<double>(((static_cast<double>(dxdy) * static_cast<int16_t>((ptC.y - ptL1.y)) + static_cast<double>(dx2) * ptC.x + static_cast<double>(dy2) * ptL1.x) / (dx2 + dy2)));
        xI = static_cast<int32_t>(dxI);
    } else {
        xI = static_cast<int32_t>((static_cast<int32_t>((static_cast<uint32_t>((dxdy * static_cast<int16_t>((ptC.y - ptL1.y)))) + static_cast<uint32_t>((dx2 * ptC.x)) + static_cast<uint32_t>((dy2 * ptL1.x)))) / (dx2 + dy2)));
    }
    if (dx != 0) {
        yI = ptL1.y + static_cast<int32_t>((static_cast<int32_t>(((xI - ptL1.x) * dy)) / dx));
    } else {
        /* A vertical route's closest point is level with the field's center.
           The original used ptL1.y, measuring from the route's start. */
        yI = ptC.y;
    }
    dxI = xI - ptC.x;
    dyI = yI - ptC.y;
    r2I = static_cast<uint32_t>((dxI * dxI)) + static_cast<uint32_t>((dyI * dyI));
    if (r2 <= r2I) {
        return 0;
    }
    dxT = xI - ptL1.x;
    dyT = yI - ptL1.y;
    lT = static_cast<uint32_t>((dxT * dxT)) + static_cast<uint32_t>((dyT * dyT));
    dCtr = LOWORD(static_cast<int32_t>(sqrt(static_cast<double>(lT))));
    lT = r2 - r2I;
    if (lT <= 0) {
        return 0;
    }
    dOff = LOWORD(static_cast<int32_t>(sqrt(static_cast<double>(lT))));
    if (ptL1.x < ptL2.x) {
        if (xI < ptL1.x) {
            dCtr = -dCtr;
        }
    } else if (ptL1.x > ptL2.x) {
        if (xI > ptL1.x) {
            dCtr = -dCtr;
        }
    } else if (ptL1.y < ptL2.y) {
        if (yI < ptL1.y) {
            dCtr = -dCtr;
        }
    } else if (yI > ptL1.y) {
        dCtr = -dCtr;
    }
    *pdStart = 0 <= dCtr - dOff ? dCtr - dOff : 0;
    *pdEnd = dMax >= dCtr + dOff ? dCtr + dOff : dMax;
    if (*pdEnd <= 0 || *pdStart >= dMax) {
        return 0;
    }
    return 1;
}

void IntToRoman(int16_t i, char *pszOut) {
    if (i <= 0) {
        *pszOut = 0;
    } else {
        for (; i >= 10; i -= 10) {
            *pszOut++ = 'X';
        }
        if (i == 9) {
            *pszOut++ = 'I';
            *pszOut++ = 'X';
        } else {
            if (i >= 4) {
                if (i == 4) {
                    *pszOut++ = 'I';
                }
                *pszOut++ = 'V';
                i -= 5;
            }
            while (i-- > 0) {
                *pszOut++ = 'I';
            }
        }
        *pszOut = 0;
    }
    return;
}

int16_t FCheckPassword(APPSESSION &app) {
    int32_t lSaltDef;

    if (app.state.lSaltCur == 0 || app.state.lSaltLast == app.state.lSaltCur || app.state.fAi) {
        return 1;
    }
    if (app.state.vszDefPass[0] != 0) {
        lSaltDef = LSaltFromSz(app.state.vszDefPass);
        if (lSaltDef == app.state.lSaltCur) {
            return 1;
        }
    }
    if (app.state.ini.fValidate) {
        return 0;
    }
    return PlayerChoiceIO().pfnPromptPassword(app);
}

int32_t LSaltFromSz(const char *psz) {
    int32_t lSalt;

    lSalt = 0;
    if (*psz == 0) {
        return 0;
    }
    while (*psz != 0) {
        // Original x86 arithmetic wraps at 32 bits and treats bytes as signed.
        lSalt = static_cast<int32_t>((static_cast<uint32_t>(lSalt) + static_cast<uint32_t>(static_cast<int32_t>(static_cast<int16_t>(*psz)))));
        psz++;
        if (*psz != 0) {
            lSalt = static_cast<int32_t>((static_cast<uint32_t>(lSalt) * static_cast<uint32_t>(static_cast<int32_t>(static_cast<int16_t>(*psz)))));
            psz++;
        }
    }
    if (lSalt == 0) {
        lSalt = 1;
    }
    return lSalt;
}

int32_t LDistance2(POINT16 pt1, POINT16 pt2) {
    int32_t dy;
    int32_t dx;

    dx = static_cast<int16_t>((pt1.x - pt2.x));
    dy = static_cast<int16_t>((pt1.y - pt2.y));
    return static_cast<uint32_t>((dx * dx)) + static_cast<uint32_t>((dy * dy));
}

char *PszGetLine(char **ppszBeg) {
    char *pszStart;
    char *psz;

    for (psz = *ppszBeg; *psz == ' '; psz++) {
    }
    pszStart = psz;
    for (; *psz != 0 && *psz != '\n' && *psz != '\r'; psz++) {
    }
    if (*psz == '\r' && psz[1] == '\n') {
        *ppszBeg = psz + 2;
    } else {
        *ppszBeg = psz + 1;
    }
    *psz = 0;
    return pszStart;
}

int16_t CParseNumbers(char *psz, int32_t *pl, int16_t cMax) {
    int16_t iRead;
    int16_t fValid;
    int32_t lNum;

    iRead = 0;
    lNum = 0;
    fValid = 0;
    for (; iRead < cMax && *psz != 0; psz++) {
        if (*psz != ' ' && (*psz < '0' || *psz > '9')) {
            return -1;
        }
        if (*psz != ' ') {
            fValid = 1;
            lNum = static_cast<uint32_t>((lNum * 10)) + static_cast<int16_t>((*psz - '0'));
        } else if (fValid) {
            pl[iRead++] = lNum;
            lNum = 0;
            fValid = 0;
        }
    }
    if (fValid) {
        pl[iRead++] = lNum;
    }
    return iRead;
}
