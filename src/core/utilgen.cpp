#include "common.h"
#include <limits>
#include "appstate.h"
#include "text_io.h"
#include "player_choice_io.h"
#include "record_io.h"

std::int16_t rgPrimes[128] = {3,   5,   7,   11,  13,  17,  19,  23,  29,  31,  37,  41,  43,  47,  53,  59,  61,  67,  71,  73,  79,  83,  89,  97,  101, 103,
                         107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241,
                         251, 257, 263, 279, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401,
                         409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563, 569, 571,
                         577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701, 709, 719, 727};

void PushRandom(APPSESSION &app, std::int32_t lNew1, std::int32_t lNew2) {
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

void Randomize(APPSESSION &app, std::uint32_t dw) {
    std::int16_t a;
    std::int16_t b;

    a = LOWORD(dw) & 0x3f;
    b = LOWORD(static_cast<std::uint32_t>((dw >> 6))) & 0x3f;
    if (a == b) {
        b = (b + 1) & 0x3f;
    }
    app.state.lRandSeed1 = rgPrimes[a];
    app.state.lRandSeed2 = rgPrimes[b];
    return;
}

void Randomize2(APPSESSION &app, std::uint32_t dw) {
    std::int16_t a;
    std::int16_t b;

    a = LOWORD(dw) & 0x7f;
    b = LOWORD(static_cast<std::uint32_t>((dw >> 7))) & 0x7f;
    a ^= 0x35;
    b ^= 0x5c;
    if (a == b) {
        b = (b + 1) & 0x7f;
    }
    app.state.lRandSeed1 = rgPrimes[a];
    app.state.lRandSeed2 = rgPrimes[b];
    return;
}

std::int16_t Random(APPSESSION &app, std::int16_t c) {
    std::int32_t z;
    std::int32_t s1;
    std::int32_t k;
    std::int32_t s2;

    s1 = app.state.lRandSeed1;
    s2 = app.state.lRandSeed2;
    k = static_cast<std::int32_t>((s1 / 53668));
    s1 = static_cast<std::uint32_t>(((s1 - static_cast<std::uint32_t>((k * 53668))) * 0x9c4e)) - static_cast<std::uint32_t>((k * 12211));
    if (s1 < 0) {
        s1 += 2147483563;
    }
    k = static_cast<std::int32_t>((s2 / 52774));
    s2 = static_cast<std::uint32_t>(((s2 - static_cast<std::uint32_t>((k * 52774))) * 0x9ef4)) - static_cast<std::uint32_t>((k * 3791));
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
    return LOWORD(static_cast<std::uint32_t>((static_cast<std::uint32_t>(z) % c)));
}

void GetFileSeeds(APPSESSION &app, std::int32_t *pl1, std::int32_t *pl2) {
    *pl1 = app.state.lFileSeed1;
    *pl2 = app.state.lFileSeed2;
    return;
}

void SetFileSeeds(APPSESSION &app, std::int32_t l1, std::int32_t l2) {
    app.state.lFileSeed1 = l1;
    app.state.lFileSeed2 = l2;
    return;
}

void SetFileXorStream(APPSESSION &app, std::int32_t lid, std::int16_t lSalt, std::int16_t turn, std::int16_t iPlayer, std::int16_t fCrippled) {
    std::int16_t a;
    std::int16_t b;

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

std::int32_t LGetNextFileXor(APPSESSION &app) {
    std::int32_t s1;
    std::int32_t k;
    std::int32_t s2;

    s1 = app.state.lFileSeed1;
    s2 = app.state.lFileSeed2;
    k = static_cast<std::int32_t>((s1 / 53668));
    s1 = static_cast<std::uint32_t>(((s1 - static_cast<std::uint32_t>((k * 53668))) * 0x9c4e)) - static_cast<std::uint32_t>((k * 12211));
    if (s1 < 0) {
        s1 += 2147483563;
    }
    k = static_cast<std::int32_t>((s2 / 52774));
    s2 = static_cast<std::uint32_t>(((s2 - static_cast<std::uint32_t>((k * 52774))) * 0x9ef4)) - static_cast<std::uint32_t>((k * 3791));
    if (s2 < 0) {
        s2 += 2147483399;
    }
    app.state.lFileSeed1 = s1;
    app.state.lFileSeed2 = s2;
    return s1 - s2;
}

void XorFileBuf(APPSESSION &app, char *rgb, std::int16_t cb) {
    char    *plMac;
    char    *pl;
    std::int32_t  lPrev;
    std::uint32_t ul;
    char    *pch;

    lPrev = 0;
    pl = rgb;
    plMac = pl + (cb >> 2) * 4;
    for (; pl < plMac; pl += 4) {
        lPrev = LGetNextFileXor(app);
        NATIVEBYTEREADER(std::as_bytes(std::span(pl, 4))).FReadU32Le(ul);
        ul ^= static_cast<std::uint32_t>(lPrev);
        NATIVEBYTEWRITER(std::as_writable_bytes(std::span(pl, 4))).FWriteU32Le(ul);
    }
    cb &= 3;
    if (cb > 0) {
        pch = static_cast<char *>(pl);
        lPrev = LGetNextFileXor(app);
        while (cb-- != 0) {
            *pch ^= static_cast<std::int16_t>(static_cast<std::int8_t>(LOBYTE(LOWORD(lPrev) & 0xff)));
            lPrev = static_cast<std::int32_t>((lPrev >> 8));
            pch++;
        }
    }
    return;
}

int ICompLong(APPSESSION &, std::int32_t *pl1, std::int32_t *pl2) { return static_cast<std::int16_t>((*pl1 - *pl2)); }

std::array<char, 19> PszGetCompressedPlanet(APPSESSION &app, std::int16_t id) {
    std::array<char, 19> szResult{};
    std::int16_t  fCap;
    std::int16_t  iOffset;
    std::int16_t  fHigh;
    std::int16_t  iChunk;
    std::int16_t  i;
    std::int16_t  iBuild;
    std::int16_t  iNibble;
    std::uint8_t *pchLen;
    char    *pszOut;
    std::uint8_t *pch;
    std::int16_t  iLen;

    iNibble = 0;
    if (id >= cPlanetName) {
        id %= cPlanetName;
    }
    if (id == app.state.iLastGet) {
        CchCopyUtf8Prefix(std::span{szResult}, app.state.szLastGet);
        return szResult;
    }
    iChunk = id >> 6;
    iOffset = id & 0x3f;
    pch = reinterpret_cast<std::uint8_t *>(&aPNCmpr[aiPNChunkOffset[iChunk]]);
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
    CchCopyUtf8Prefix(std::span{szResult}, app.state.szLastGet);
    return szResult;
}

void OutputFileString(const char *szFile, std::string_view sz) {
    NATIVEFILEOPENOPTIONS w;
    std::int16_t  hf;

    w.mdAccess = NATIVEFILEACCESS::readWrite;
    if (!FFileExists(szFile)) {
        w.fCreate = w.fTruncate = true;
    }
    hf = HfOpenFile(szFile, w).hf;
    if (hf != -1) {
        LSeekFile(hf, 0, NATIVESEEKORIGIN::end);
        CbWriteFile(hf, std::as_bytes(std::span{sz.data(), sz.size()}));
        CloseFile(hf);
    }
    return;
}

void StarsCopyFile(APPSESSION &app, const char *szSrc, const char *szDst) {
    char     rgb[2048];
    std::int16_t  hfDst;
    std::int64_t cb;
    std::optional<std::int64_t> cbFile;

    hfDst = -1;
    StarsRestore<std::int16_t> restoreSilent(app.state.fFileErrSilent);
    StarsFileGuard closeDestination(hfDst);
    StarsRecoveryScope scopeRecovery;
    app.state.fFileErrSilent = 1;
    try {
        RecordIO().pfnStreamOpen(app, szSrc, mdRead);
        hfDst = HfOpenFile(szDst, {.mdAccess = NATIVEFILEACCESS::readWrite, .mdShare = NATIVEFILESHARE::exclusive, .fCreate = true, .fTruncate = true}).hf;
        if (hfDst == -1)
            goto LStreamError;
        cbFile = CbFileSize(app.state.hf);
        if (!cbFile) {
            FileError(app, idsGameFileAppearsCorruptUnableLoadFile);
            goto LStreamError;
        }
        for (cb = *cbFile; cb > 2048; cb -= 2048) {
            RgFromStream(app, std::as_writable_bytes(std::span{rgb}).first(static_cast<std::size_t>(0x800)));
            if (CbWriteFile(hfDst, std::as_bytes(std::span{rgb}).first(static_cast<std::size_t>(0x800))) != 0x800)
                goto LStreamError;
        }
        if (cb != 0) {
            RgFromStream(app, std::as_writable_bytes(std::span{rgb}).first(static_cast<std::size_t>(cb)));
            CbWriteFile(hfDst, std::as_bytes(std::span{rgb}).first(static_cast<std::size_t>(cb)));
        }
    } catch (const StarsError &) {
    }
LStreamError:
    RecordIO().pfnStreamClose(app);
    return;
}

std::int16_t AlertSz(APPSESSION &app, const char *sz, std::int16_t mbType) {
    char szT[256];

    if (app.state.ini.fValidate || (app.state.ini.fLogging && app.state.ini.fGen)) {
        CchSprintf(szT, "Error: %s", sz);
        OutputSz(app, !app.state.ini.fValidate ? 6 : 7, szT);
        return win32::IDYES;
    }
    return IdAlertBox(sz, mbType);
}

std::int16_t CchGetString(APPSESSION &app, StringId ids, std::span<char> psz) {
    if (psz.empty()) return 0;
    const auto szCatalog = TextIO().pfnCompressed(app, ids);
    const std::string_view szT(szCatalog.data());
    const std::size_t cch = CchUtf8Prefix(szT, psz.size() - 1);

    std::memmove(psz.data(), szT.data(), cch);
    psz[cch] = 0;
    return static_cast<std::int16_t>(cch);
}

std::array<char, 12> PszFromInt(APPSESSION &app, std::int16_t i) {
    std::array<char, 12> psz{};

    CchSprintf(psz, app.state.PCTD, i);
    return psz;
}

std::array<char, 12> PszFromLong(APPSESSION &app, std::int32_t l) {
    std::array<char, 12> psz{};

    CchSprintf(psz, app.state.PCTLD, l);
    return psz;
}

std::array<char, 12> PszFromLongK(APPSESSION &app, std::int32_t l) {
    std::int16_t fExtraLarge;
    std::int16_t fLarge;
    std::int16_t cch;
    std::array<char, 12> psz{};

    fLarge = l >= 10000;
    fExtraLarge = l >= 1000000;
    if (fExtraLarge) {
        // Round before clamping without overflowing the signed 32-bit input.
        l = static_cast<std::int32_t>((static_cast<std::int64_t>(l) + 500000) / 0xf4240);
        if (l > 999) {
            l = 999;
        }
    } else if (fLarge) {
        l = static_cast<std::int32_t>(((l + 500) / 1000));
        if (l > 999) {
            l = 999;
        }
    }
    psz = PszFromInt(app, LOWORD(l));
    cch = static_cast<std::int16_t>(std::strlen(psz.data()));
    if (fExtraLarge) {
        psz[cch++] = 'M';
    } else if (fLarge) {
        psz[cch++] = 'k';
    }
    psz[cch] = 0;
    return psz;
}

std::array<char, 15> CommaFormatLong(APPSESSION &app, std::int32_t l) {
    char    rgch[15];
    // Signed 32-bit decimal text needs at most 14 grouped bytes plus NUL.
    std::array<char, 15> psz{};
    std::int16_t c;
    std::int16_t cSkip;
    char   *pchOut;
    char   *pch;

    c = CchSprintf(rgch, app.state.PCTLD, l);
    pch = rgch;
    pchOut = psz.data();
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
    return psz;
}

void BoundPoints(RECT *prc, POINT16 *rgpt, std::int16_t cpt) {
    std::int16_t ipt;
    std::int16_t xMax;
    std::int16_t yMax;
    std::int16_t xMin;
    std::int16_t yMin;

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

std::optional<std::size_t> FCompressUserString(APPSESSION &app, std::string_view szIn, std::span<std::byte> szOut) {
    std::int16_t fHalf;
    char    szWork[1024];
    std::int16_t iNyb;
    char   *pchOut;
    std::int16_t cNyb;
    std::size_t ich, cOut;

    fHalf = 0;
    pchOut = szWork;
    if (szIn.find(char{0}) != std::string_view::npos) return std::nullopt;
    for (ich = 0; ich < szIn.size(); ich++) {
        iNyb = NybbleFromCh(app, static_cast<std::uint8_t>(szIn[ich]));
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
                    return std::nullopt;
                }
            }
            iNyb >>= 4;
        }
    }
    if (fHalf) {
        *pchOut |= 0xf;
        pchOut++;
    }
    if (static_cast<std::size_t>(pchOut - szWork) > szOut.size()) {
        return std::nullopt;
    }
    cOut = static_cast<std::size_t>(pchOut - szWork);
    if (cOut != 0) std::memmove(szOut.data(), szWork, cOut);
    return cOut;
}

static std::optional<std::size_t> FDecompressUserStringInternal(APPSESSION &app, std::span<const std::byte> rgbIn, std::span<char> szOut, bool fUtf8) {
    std::int16_t fHalf;
    std::int16_t cIn;
    const char *szIn = reinterpret_cast<const char *>(rgbIn.data());
    char    szWork[1024];
    std::int16_t iNyb;
    char   *pchOut;
    std::size_t cOut;

    fHalf = 0;
    pchOut = szWork;
    if (szOut.empty()) return std::nullopt;
    if (rgbIn.size() > static_cast<std::size_t>(std::numeric_limits<std::int16_t>::max()) || szOut.size() > sizeof(szWork)) goto invalid;
    cIn = static_cast<std::int16_t>(rgbIn.size());
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
        if (static_cast<std::size_t>(pchOut - szWork) >= szOut.size()) {
            goto invalid;
        }
    }
    *pchOut = 0;
    if (std::memchr(szWork, 0, static_cast<std::size_t>((pchOut - szWork))) != nullptr) goto invalid;
    if (fUtf8 && !FUtf8Valid(szWork, static_cast<std::size_t>((pchOut - szWork)))) goto invalid;
    cOut = static_cast<std::size_t>(pchOut - szWork);
    std::memmove(szOut.data(), szWork, cOut + 1);
    return cOut;
invalid:
    szOut[0] = 0;
    return std::nullopt;
}

std::optional<std::size_t> FDecompressUserString(APPSESSION &app, std::span<const std::byte> szIn, std::span<char> szOut) {
    return FDecompressUserStringInternal(app, szIn, szOut, true);
}

std::optional<std::size_t> FDecompressUserStringBytes(APPSESSION &app, std::span<const std::byte> szIn, std::span<char> szOut) {
    return FDecompressUserStringInternal(app, szIn, szOut, false);
}

std::int16_t NybbleFromCh(APPSESSION &app, std::uint8_t ch) {
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
    pch = std::strchr(app.state.rgchcomp, ch);
    if (pch) {
        return (pch - app.state.rgchcomp + 4) << 4 | 0xe;
    }
    return ch << 4 | 0xf;
}

char ChFromNybble(APPSESSION &app, std::int16_t nyb) {
    std::int16_t iPage;
    std::int16_t iVal;

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

std::int16_t FIntersectCircleLine(POINT16 ptL1, POINT16 ptL2, POINT16 ptC, std::int32_t r2, std::int16_t dMax, std::int16_t *pdStart, std::int16_t *pdEnd) {
    std::int32_t dyT;
    std::int32_t dxdy;
    std::int16_t dCtr;
    std::int32_t dy2;
    std::int32_t dy;
    std::int32_t yI;
    std::int32_t dyI;
    std::int32_t dxT;
    std::int32_t lT;
    std::int32_t dx2;
    std::int16_t dOff;
    std::int32_t dx;
    std::int32_t dxI;
    std::int32_t xI;
    std::int32_t r2I;

    dx = static_cast<std::int16_t>((ptL2.x - ptL1.x));
    dy = static_cast<std::int16_t>((ptL2.y - ptL1.y));
    dxdy = static_cast<std::uint32_t>((dx * dy));
    dx2 = static_cast<std::uint32_t>((dx * dx));
    dy2 = static_cast<std::uint32_t>((dy * dy));
    if (dxdy > 500000 || dx2 > 500000 || dy2 > 500000) {
        dxI = static_cast<double>(((static_cast<double>(dxdy) * static_cast<std::int16_t>((ptC.y - ptL1.y)) + static_cast<double>(dx2) * ptC.x + static_cast<double>(dy2) * ptL1.x) / (dx2 + dy2)));
        xI = static_cast<std::int32_t>(dxI);
    } else {
        xI = static_cast<std::int32_t>((static_cast<std::int32_t>((static_cast<std::uint32_t>((dxdy * static_cast<std::int16_t>((ptC.y - ptL1.y)))) + static_cast<std::uint32_t>((dx2 * ptC.x)) + static_cast<std::uint32_t>((dy2 * ptL1.x)))) / (dx2 + dy2)));
    }
    if (dx != 0) {
        yI = ptL1.y + static_cast<std::int32_t>((static_cast<std::int32_t>(((xI - ptL1.x) * dy)) / dx));
    } else {
        /* A vertical route's closest point is level with the field's center.
           The original used ptL1.y, measuring from the route's start. */
        yI = ptC.y;
    }
    dxI = xI - ptC.x;
    dyI = yI - ptC.y;
    r2I = static_cast<std::uint32_t>((dxI * dxI)) + static_cast<std::uint32_t>((dyI * dyI));
    if (r2 <= r2I) {
        return 0;
    }
    dxT = xI - ptL1.x;
    dyT = yI - ptL1.y;
    lT = static_cast<std::uint32_t>((dxT * dxT)) + static_cast<std::uint32_t>((dyT * dyT));
    dCtr = LOWORD(static_cast<std::int32_t>(std::sqrt(static_cast<double>(lT))));
    lT = r2 - r2I;
    if (lT <= 0) {
        return 0;
    }
    dOff = LOWORD(static_cast<std::int32_t>(std::sqrt(static_cast<double>(lT))));
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

std::optional<std::size_t> IntToRoman(std::int16_t i, std::span<char> pszOut) {
    constexpr std::string_view rgszOnes[] = {"", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX"};
    std::size_t c;
    std::size_t cTens;
    std::string_view szOnes;

    cTens = i > 0 ? static_cast<std::size_t>(i / 10) : 0;
    szOnes = rgszOnes[i > 0 ? i % 10 : 0];
    c = cTens + szOnes.size();
    if (pszOut.size() <= c) {
        if (!pszOut.empty()) pszOut[0] = 0;
        return std::nullopt;
    }
    std::fill_n(pszOut.begin(), cTens, 'X');
    std::copy(szOnes.begin(), szOnes.end(), pszOut.begin() + cTens);
    pszOut[c] = 0;
    return c;
}

std::int16_t FCheckPassword(APPSESSION &app) {
    std::int32_t lSaltDef;

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

std::int32_t LSaltFromSz(const char *psz) {
    std::int32_t lSalt;

    lSalt = 0;
    if (*psz == 0) {
        return 0;
    }
    while (*psz != 0) {
        // Original x86 arithmetic wraps at 32 bits and treats bytes as signed.
        lSalt = static_cast<std::int32_t>((static_cast<std::uint32_t>(lSalt) + static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(*psz)))));
        psz++;
        if (*psz != 0) {
            lSalt = static_cast<std::int32_t>((static_cast<std::uint32_t>(lSalt) * static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(*psz)))));
            psz++;
        }
    }
    if (lSalt == 0) {
        lSalt = 1;
    }
    return lSalt;
}

std::int32_t LDistance2(POINT16 pt1, POINT16 pt2) {
    std::int32_t dy;
    std::int32_t dx;

    dx = static_cast<std::int16_t>((pt1.x - pt2.x));
    dy = static_cast<std::int16_t>((pt1.y - pt2.y));
    return static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
}

std::string_view PszGetLine(std::string_view &ppszBeg) {
    std::size_t ich;
    std::size_t cch;
    std::size_t cchBreak;
    std::string_view pszStart;

    ich = ppszBeg.find_first_not_of(' ');
    if (ich == std::string_view::npos) ich = ppszBeg.size();
    ppszBeg.remove_prefix(ich);
    cch = ppszBeg.find_first_of(std::string_view("\0\r\n", 3));
    if (cch == std::string_view::npos) cch = ppszBeg.size();
    pszStart = ppszBeg.substr(0, cch);
    cchBreak = 0;
    if (cch < ppszBeg.size()) {
        cchBreak = 1;
        if (ppszBeg[cch] == '\r' && cch + 1 < ppszBeg.size() && ppszBeg[cch + 1] == '\n') cchBreak = 2;
    }
    ppszBeg.remove_prefix(cch + cchBreak);
    return pszStart;
}

std::optional<std::size_t> CParseNumbers(std::string_view psz, std::span<std::int32_t> pl) {
    std::size_t iRead;
    std::size_t ich;
    bool fValid;
    std::uint32_t lNum;
    char ch;

    iRead = 0;
    lNum = 0;
    fValid = false;
    for (ich = 0; iRead < pl.size() && ich < psz.size() && psz[ich] != 0; ich++) {
        ch = psz[ich];
        if (ch != ' ' && (ch < '0' || ch > '9')) {
            return std::nullopt;
        }
        if (ch != ' ') {
            fValid = true;
            lNum = lNum * 10 + static_cast<std::uint32_t>(ch - '0');
        } else if (fValid) {
            pl[iRead++] = static_cast<std::int32_t>(lNum);
            lNum = 0;
            fValid = false;
        }
    }
    if (fValid) {
        pl[iRead++] = static_cast<std::int32_t>(lNum);
    }
    return iRead;
}
