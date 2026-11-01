#include "common.h"
#include "ownership.h"

#include <algorithm>
#include <cerrno>
#include <ctime>

// qsortSwap16 exchanges complete elements, including native-width pointers.
static void qsortSwap16(unsigned char *a, unsigned char *b, std::size_t width) {
    while (width != 0) {
        --width;
        unsigned char value = a[width];
        a[width] = b[width];
        b[width] = value;
    }
}

// qsort16 preserves the ordering of equal keys in Stars' Win16 CRT qsort.
// Reconstructed from 0024:0866-09c6, matching qsort.asm in C700 and MSVC
// MLIBCEW.LIB. The initial sorted check, first-element pivot, and strict
// partition boundaries matter: ICompLong compares planet X coordinates only.
void qsort16(APPSESSION &app, void *base, std::size_t count, std::size_t width, int (*compare)(APPSESSION &, const void *, const void *)) {
    if (count < 2 || width == 0)
        return;

    unsigned char *items = static_cast<unsigned char *>(base);
    std::size_t         i;
    for (i = 1; i < count; ++i) {
        if (static_cast<std::int16_t>(compare(app, items + i * width, items + (i - 1) * width)) < 0)
            break;
    }
    if (i == count)
        return;

    // Process the smaller partition first, bounding pending ranges by log2(count).
    std::size_t lows[sizeof(std::size_t) * CHAR_BIT], highs[sizeof(std::size_t) * CHAR_BIT];
    std::size_t pending = 0, lo = 0, hi = count - 1;
    for (;;) {
        if (lo < hi) {
            std::size_t left = lo, right = hi + 1;
            std::size_t lastLess = lo, firstGreater = hi;
            for (;;) {
                while (++left != hi) {
                    int order = static_cast<std::int16_t>(compare(app, items + left * width, items + lo * width));
                    if (order > 0)
                        break;
                    if (order < 0)
                        lastLess = left;
                }
                for (;;) {
                    --right;
                    int order = static_cast<std::int16_t>(compare(app, items + lo * width, items + right * width));
                    if (order > 0)
                        break;
                    if (order < 0)
                        firstGreater = right;
                    else if (right == lo)
                        break;
                }
                if (right <= left)
                    break;
                qsortSwap16(items + left * width, items + right * width, width);
                firstGreater = right;
                lastLess = left;
            }
            qsortSwap16(items + lo * width, items + right * width, width);

            if (hi - firstGreater >= lastLess - lo) {
                lows[pending] = firstGreater;
                highs[pending++] = hi;
                hi = lastLess;
            } else {
                lows[pending] = lo;
                highs[pending++] = lastLess;
                lo = firstGreater;
            }
        } else {
            if (pending == 0)
                return;
            lo = lows[--pending];
            hi = highs[pending];
        }
    }
}

// CchSprintf drops the l from each %ld, %li, %lu, %lx or %lX in szFormat
// before formatting, since its arguments are 32-bit. Like wsprintf, it
// writes at most the destination size or 1024 bytes, the terminator included,
// and returns the characters it wrote.
int CchSprintfFields(std::span<char> sz, const char *szFormat, std::span<const NATIVEFORMATARG> args) {
    char        szFmtSmall[256];
    char       *szFmt;
    std::size_t      cbFmt;
    const char *pchIn;
    char       *pchOut;
    int         cch;
    NATIVEOWNER<char, std::free> format;
    const std::size_t cb = std::min(sz.size(), std::size_t{1024});

    if (sz.empty()) return 0;

    // Dropping l only shortens the format, so a copy its size holds it.
    cbFmt = std::strlen(szFormat) + 1;
    szFmt = cbFmt <= sizeof(szFmtSmall) ? szFmtSmall : static_cast<char *>(std::malloc(cbFmt));
    format.reset(szFmt == szFmtSmall ? nullptr : szFmt);
    if (!szFmt) {
        sz[0] = 0;
        return 0;
    }
    pchIn = szFormat;
    pchOut = szFmt;
    while (*pchIn != 0) {
        *pchOut++ = *pchIn;
        if (*pchIn++ != '%') {
            continue;
        }
        while (*pchIn != 0 && std::strchr("-+ #0123456789.", *pchIn)) {
            *pchOut++ = *pchIn++;
        }
        if (*pchIn == 'l' && pchIn[1] != 0 && std::strchr("diuxX", pchIn[1])) {
            pchIn++;
        }
        if (*pchIn != 0) {
            *pchOut++ = *pchIn++;
        }
    }
    *pchOut = 0;
    cch = CchFormatNative(sz.first(cb), szFmt, args);
    format.reset();
    if (cch < 0) {
        sz[0] = 0;
        return 0;
    }
    if (static_cast<std::size_t>(cch) >= cb) {
        cch = static_cast<int>(CchUtf8Prefix(std::string_view(sz.data(), cb - 1), cb - 1));
        sz[cch] = 0;
    }
    return cch;
}

// LMulDiv returns lNumber * lNumerator / lDenominator rounded half away from
// zero, or -1 for a zero denominator or a result out of range.
std::int32_t LMulDiv(std::int32_t lNumber, std::int32_t lNumerator, std::int32_t lDenominator) {
    std::int64_t l;

    if (lDenominator == 0) {
        return -1;
    }
    if (lDenominator < 0) {
        lNumber = -lNumber;
        lDenominator = -lDenominator;
    }
    if ((lNumber < 0) == (lNumerator < 0)) {
        l = (static_cast<std::int64_t>(lNumber) * lNumerator + lDenominator / 2) / lDenominator;
    } else {
        l = (static_cast<std::int64_t>(lNumber) * lNumerator - lDenominator / 2) / lDenominator;
    }
    if (l > 2147483647 || l < -2147483647) {
        return -1;
    }
    return static_cast<std::int32_t>(l);
}

#ifdef _WIN32


void GetDateTimeSz(char *szDate, char *szTime) {
    _strdate(szDate);
    _strtime(szTime);
}

std::int16_t FSzPrefixNoCase(const char *sz1, const char *sz2, std::int16_t cch) { return _strnicmp(sz1, sz2, cch) == 0; }

#else

std::uint32_t DwTickCount() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::uint32_t>((static_cast<std::uint64_t>(ts.tv_sec) * 1000 + static_cast<std::uint64_t>(ts.tv_nsec) / 1000000));
}

void GetDateTimeSz(char *szDate, char *szTime) {
    std::time_t     t;
    struct std::tm *ptm;

    t = std::time(nullptr);
    ptm = std::localtime(&t);
    std::strftime(szDate, 9, "%m/%d/%y", ptm);
    std::strftime(szTime, 9, "%H:%M:%S", ptm);
}

std::int16_t FSzPrefixNoCase(const char *sz1, const char *sz2, std::int16_t cch) { return strncasecmp(sz1, sz2, cch) == 0; }

#endif
std::int64_t LNativeTime(void) { return static_cast<std::int64_t>(std::time(nullptr)); }



// Game entrypoints retain their small handles;
// resource ownership, binary streams and native exceptions live in fileio.cpp.
NATIVEFILEOPENRESULT HfOpenFile(const char *szFile, NATIVEFILEOPENOPTIONS nfo) {
    return HfNativeOpenLegacy(szFile, nfo);
}
std::optional<std::size_t> CbReadFile(std::int16_t hf, std::span<std::byte> rg) { return CbNativeReadLegacy(hf, rg); }
std::optional<std::size_t> CbWriteFile(std::int16_t hf, std::span<const std::byte> rg) { return CbNativeWriteLegacy(hf, rg); }
std::int32_t LSeekFile(std::int16_t hf, std::int32_t l, NATIVESEEKORIGIN iOrigin) { return LNativeSeekLegacy(hf, l, iOrigin); }
std::int32_t CbFileSize(std::int16_t hf) { return CbNativeSizeLegacy(hf); }
void CloseFile(std::int16_t hf) { CloseNativeLegacy(hf); }
std::int16_t FFileExists(const char *szFile) { return IAccessNativePath(szFile, 0) == 0; }
std::int16_t FFileReadOnly(const char *szFile) { return IAccessNativePath(szFile, 2) != 0; }
void MakeDir(const char *szDir) { ICreateNativeDirectory(szDir, 0777); }
int CreateNativeDir(const char *pszDir) { return ICreateNativeDirectory(pszDir, 0777); }
std::int16_t FEnsureNativeDir(const char *pszDir) { return FEnsureNativeDirectory(pszDir); }
std::int16_t FNativeEnumerateDirectory(APPSESSION &app, const char *pszDir, FNativeDirectoryEntryProc pfnEntry, void *lpData) {
    NATIVEDIR *pdir;
    const char *pszEntry;
    if (!pfnEntry || !(pdir = PdirNativeOpen(pszDir))) return 0;
    StarsNativeDirGuard closeDirectory(pdir);
    for (;;) {
        errno = 0;
        pszEntry = PszNativeReadDir(pdir);
        if (!pszEntry) return errno == 0;
        if (!FUtf8Valid(pszEntry, std::strlen(pszEntry))) { errno = EILSEQ; return 0; }
        if (!pfnEntry(app, lpData, pszEntry)) return 1;
    }
}
std::optional<std::int64_t> FNativeFileSize(const char *pszFile) {
    const auto info = IGetNativeFileInfo(pszFile, NATIVELINKMODE::follow);
    if (!info || !info->fRegular) return std::nullopt;
    return info->cb;
}
std::int16_t FNativeDirectoryExists(const char *pszDir) {
    const auto info = IGetNativeFileInfo(pszDir, NATIVELINKMODE::follow);
    return info && info->fDirectory;
}
std::optional<NATIVEPATHINFO> FNativePathInfo(const char *psz) {
    const auto info = IGetNativeFileInfo(psz, NATIVELINKMODE::inspect);
    if (!info) return std::nullopt;
    return NATIVEPATHINFO{static_cast<bool>(info->fDirectory), static_cast<bool>(info->fLink)};
}
NATIVEFILE *PfOpenNativeFile(const char *psz, NATIVEFILEMODE mode) { return PfNativeOpen(psz, mode); }
