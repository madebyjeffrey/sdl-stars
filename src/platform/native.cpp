#include "common.h"
#include "ownership.h"

#include <algorithm>
#include <cerrno>
#include <ctime>
#include <limits>
#include <new>
#include <string>

// qsortSwap16 exchanges complete elements, including native-width pointers.
static void qsortSwap16(unsigned char *a, unsigned char *b, std::size_t width) {
    while (width != 0) {
        --width;
        unsigned char value = a[width];
        a[width] = b[width];
        b[width] = value;
    }
}

template <typename T>
static void Qsort16Typed(APPSESSION &app, std::span<T> rg, int (*compare)(APPSESSION &, T *, T *)) {
    std::size_t count = rg.size();
    if (count < 2)
        return;

    T *items = rg.data();
    std::size_t         i;
    for (i = 1; i < count; ++i) {
        if (static_cast<std::int16_t>(compare(app, items + i, items + (i - 1))) < 0)
            break;
    }
    if (i == count)
        return;

    // Process the smaller partition first, bounding pending ranges by log2(count).
    std::size_t lows[std::numeric_limits<std::size_t>::digits], highs[std::numeric_limits<std::size_t>::digits];
    std::size_t pending = 0, lo = 0, hi = count - 1;
    for (;;) {
        if (lo < hi) {
            std::size_t left = lo, right = hi + 1;
            std::size_t lastLess = lo, firstGreater = hi;
            for (;;) {
                while (++left != hi) {
                    int order = static_cast<std::int16_t>(compare(app, items + left, items + lo));
                    if (order > 0)
                        break;
                    if (order < 0)
                        lastLess = left;
                }
                for (;;) {
                    --right;
                    int order = static_cast<std::int16_t>(compare(app, items + lo, items + right));
                    if (order > 0)
                        break;
                    if (order < 0)
                        firstGreater = right;
                    else if (right == lo)
                        break;
                }
                if (right <= left)
                    break;
                qsortSwap16(reinterpret_cast<unsigned char *>(items + left), reinterpret_cast<unsigned char *>(items + right), sizeof(T));
                firstGreater = right;
                lastLess = left;
            }
            qsortSwap16(reinterpret_cast<unsigned char *>(items + lo), reinterpret_cast<unsigned char *>(items + right), sizeof(T));

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

// qsort16 preserves the ordering of equal keys in Stars' Win16 CRT qsort.
// Reconstructed from 0024:0866-09c6, matching qsort.asm in C700 and MSVC
// MLIBCEW.LIB. The initial sorted check, first-element pivot, and strict
// partition boundaries matter: ICompLong compares planet X coordinates only.
void qsort16(APPSESSION &app, std::span<POINT16> rgpt, int (*compare)(APPSESSION &, POINT16 *, POINT16 *)) {
    Qsort16Typed(app, rgpt, compare);
}

void qsort16(APPSESSION &app, std::span<std::uint16_t> rgid, int (*compare)(APPSESSION &, std::uint16_t *, std::uint16_t *)) {
    Qsort16Typed(app, rgid, compare);
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


bool GetDateTimeSz(std::span<char> szDate, std::span<char> szTime) {
    if (szDate.size() < 9 || szTime.size() < 9) return false;
    _strdate(szDate.data());
    _strtime(szTime.data());
    return true;
}

#else

std::uint32_t DwTickCount() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::uint32_t>((static_cast<std::uint64_t>(ts.tv_sec) * 1000 + static_cast<std::uint64_t>(ts.tv_nsec) / 1000000));
}

bool GetDateTimeSz(std::span<char> szDate, std::span<char> szTime) {
    std::time_t     t;
    struct std::tm *ptm;

    if (szDate.size() < 9 || szTime.size() < 9) return false;
    t = std::time(nullptr);
    ptm = std::localtime(&t);
    if (!ptm || std::strftime(szDate.data(), szDate.size(), "%m/%d/%y", ptm) != 8) return false;
    return std::strftime(szTime.data(), szTime.size(), "%H:%M:%S", ptm) == 8;
}

#endif
std::int64_t LNativeTime(void) { return static_cast<std::int64_t>(std::time(nullptr)); }



// Game entrypoints retain their small handles;
// resource ownership, binary streams and native exceptions live in fileio.cpp.
NATIVEFILEOPENRESULT HfOpenFile(std::string_view szFile, NATIVEFILEOPENOPTIONS nfo) {
    return HfNativeOpenLegacy(szFile, nfo);
}
std::optional<std::size_t> CbReadFile(std::int16_t hf, std::span<std::byte> rg) { return CbNativeReadLegacy(hf, rg); }
std::optional<std::size_t> CbWriteFile(std::int16_t hf, std::span<const std::byte> rg) { return CbNativeWriteLegacy(hf, rg); }
std::int32_t LSeekFile(std::int16_t hf, std::int32_t l, NATIVESEEKORIGIN iOrigin) { return LNativeSeekLegacy(hf, l, iOrigin); }
std::optional<std::int64_t> CbFileSize(std::int16_t hf) { return CbNativeSizeLegacy(hf); }
void CloseFile(std::int16_t hf) { CloseNativeLegacy(hf); }
bool FFileExists(std::string_view szFile) { return IAccessNativePath(szFile, 0) == 0; }
bool FFileReadOnly(std::string_view szFile) { return IAccessNativePath(szFile, 2) != 0; }
void MakeDir(std::string_view szDir) { ICreateNativeDirectory(szDir, 0777); }
int CreateNativeDir(std::string_view pszDir) { return ICreateNativeDirectory(pszDir, 0777); }
bool FEnsureNativeDir(std::string_view pszDir) { return FEnsureNativeDirectory(pszDir); }
bool FNativeEnumerateDirectory(APPSESSION &app, std::string_view pszDir, const FNativeDirectoryEntryProc &pfnEntry) {
    std::optional<std::string> szEntry;
    if (!pfnEntry) return false;
    NATIVEDIROWNER directory = PdirNativeOpen(pszDir);
    if (!directory) return false;
    for (;;) {
        errno = 0;
        szEntry = SzNativeReadDir(directory.get());
        if (!szEntry) return errno == 0;
        if (!FUtf8Valid(*szEntry)) { errno = EILSEQ; return false; }
        if (!pfnEntry(app, *szEntry)) return true;
    }
}
std::optional<std::int64_t> FNativeFileSize(std::string_view pszFile) {
    const auto info = IGetNativeFileInfo(pszFile, NATIVELINKMODE::follow);
    if (!info || !info->fRegular) return std::nullopt;
    return info->cb;
}
bool FNativeDirectoryExists(std::string_view pszDir) {
    const auto info = IGetNativeFileInfo(pszDir, NATIVELINKMODE::follow);
    return info && info->fDirectory != 0;
}
std::optional<NATIVEPATHINFO> FNativePathInfo(std::string_view psz) {
    const auto info = IGetNativeFileInfo(psz, NATIVELINKMODE::inspect);
    if (!info) return std::nullopt;
    return NATIVEPATHINFO{static_cast<bool>(info->fDirectory), static_cast<bool>(info->fLink)};
}
NATIVEFILE *PfOpenNativeFile(std::string_view psz, NATIVEFILEMODE mode) { return PfNativeOpen(psz, mode); }
