#ifndef STARS_NATIVE_H
#define STARS_NATIVE_H

class APPSESSION;

// Helpers the Win32 build needs where Stars' Win16 types and behavior differ
// from Win32. See docs/NATIVE-PORT.md.

#include <cstdint>
#include <climits>
#include <cstdlib>
#include <cstdio>
#include "encoding.h"
#include "nativeformat.h"
#include "fileio.h"
#include "native_process.h"
#include "windowtypes.h"

/*
 * Win32 basics
 *
 * The game code keeps the Win32 names it was written with. These define
 * them where windows.h is not included; the UI files include windows.h
 * first (win.h); shared numeric values live in ui_values.h.
 */

#include "ui_values.h"
#include "value.h"

/*
 * Win16 points
 *
 * Win16 POINT held 16-bit ints, and Stars writes records holding points to
 * its files. Stars' points are POINT16 to keep that layout; Win32 POINT holds
 * LONGs, so points convert where they pass into or out of the Win32 API
 * (nativeui.h).
 */

struct tagPOINT16 {
    std::int16_t x;
    std::int16_t y;
};
using POINT16 = tagPOINT16;

// qsort16 sorts like Stars' Win16 CRT qsort, which decides the order of
// elements with equal keys. Turn generation depends on that order.
void qsort16(APPSESSION &app, void *base, std::size_t count, std::size_t width, int (*compare)(APPSESSION &, const void *, const void *));

/*
 * Files
 *
 * The game reads and writes one file at a time through hf, a small handle
 * that Win16 OpenFile returned (file.c). Named open policy and result values
 * use the C++ file service, with registry ownership and UTF-8 native paths.
 * POSIX has no share modes, so a file another program has open is not refused.
 */

// The registry owns successful tokens; failure returns hf == -1 and reports
// missing only for ENOENT. The result is transient, never a saved record.
[[nodiscard]] NATIVEFILEOPENRESULT HfOpenFile(const char *szFile, NATIVEFILEOPENOPTIONS nfo);
std::int32_t CbReadFile(std::int16_t hf, void *rg, std::uint16_t cb);
std::int32_t CbWriteFile(std::int16_t hf, const void *rg, std::uint16_t cb);
// LSeekFile moves like _llseek: iOrigin 0 from the start, 1 from the
// current position, 2 from the end. It returns the new position.
std::int32_t LSeekFile(std::int16_t hf, std::int32_t l, std::int16_t iOrigin);
std::int32_t CbFileSize(std::int16_t hf);
void    CloseFile(std::int16_t hf);
std::int16_t FFileExists(const char *szFile);
std::int16_t FFileReadOnly(const char *szFile);
void    MakeDir(const char *szDir);
// Unlike MakeDir, reservation fails if the directory already exists.
int     CreateNativeDir(const char *pszDir);
using FNativeDirectoryEntryProc = std::int16_t (*)(APPSESSION &, void *lpData, const char *pszEntry);
std::int16_t FNativeEnumerateDirectory(APPSESSION &app, const char *pszDir, FNativeDirectoryEntryProc pfnEntry, void *lpData);
std::int16_t FNativeFileSize(const char *pszFile, std::int64_t *pcbFile);
std::int16_t FNativeDirectoryExists(const char *pszDir);
// Inspect the final component without following links or Windows reparse points.
std::int16_t FNativePathInfo(const char *psz, std::int16_t *pfDirectory, std::int16_t *pfLink);
std::int16_t FEnsureNativeDir(const char *pszDir);
int     RemoveNativeFile(const char *psz);
int     RemoveNativePath(const char *psz);
int     ReplaceNativeFile(const char *pszOld, const char *pszNew);
std::int64_t LNativeTime(void);
int     RenameNativeFile(const char *pszOld, const char *pszNew);
NATIVEFILE *PfOpenNativeFile(const char *psz, const char *pszMode);

// File names: the directory separator.
#ifdef _WIN32
inline constexpr char chDirSep = '\\';
inline constexpr char szDirSep[] = "\\";
#else
inline constexpr char chDirSep = '/';
inline constexpr char szDirSep[] = "/";
#endif

// GetDateTimeSz writes the date as MM/DD/YY and the time as HH:MM:SS, as
// _strdate and _strtime did, for the turn log.
void GetDateTimeSz(char *szDate, char *szTime);
// FSzPrefixNoCase compares the first cch characters, ignoring case.
std::int16_t FSzPrefixNoCase(const char *sz1, const char *sz2, std::int16_t cch);

/*
 * Formatting and arithmetic
 *
 * Win32 long is 32 bits, so the game's formats (and its string table) pass
 * int32_t values to %ld. CchSprintf formats like wsprintf with l read as 32
 * bits on every platform. LMulDiv rounds like Win32 MulDiv.
 */
int CchSprintfFields(std::span<char> sz, const char *szFormat, std::span<const NATIVEFORMATARG> rgArgs);
template<class... ARGS>
int CchSprintf(std::span<char> sz, const char *szFormat, ARGS... args) {
    const std::array<NATIVEFORMATARG, sizeof...(ARGS)> rgArgs = {NATIVEFORMATARG{args}...};
    return CchSprintfFields(sz, szFormat, rgArgs);
}
// Bounded native formatting uses standard C widths and reports truncation.
template<class... ARGS>
int CchSnprintf(std::span<char> psz, const char *pszFormat, ARGS... args) {
    return CchFormatNativeArgs(CchFormatNative, psz, pszFormat, args...);
}
std::int32_t LMulDiv(std::int32_t lNumber, std::int32_t lNumerator, std::int32_t lDenominator);


#endif
