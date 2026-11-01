#ifndef STARS_NATIVE_H
#define STARS_NATIVE_H

class APPSESSION;

// Helpers the Win32 build needs where Stars' Win16 types and behavior differ
// from Win32. See docs/NATIVE-PORT.md.

#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include "encoding.h"
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
    int16_t x;
    int16_t y;
};
using POINT16 = tagPOINT16;

// qsort16 sorts like Stars' Win16 CRT qsort, which decides the order of
// elements with equal keys. Turn generation depends on that order.
void qsort16(APPSESSION &app, void *base, size_t count, size_t width, int (*compare)(APPSESSION &, const void *, const void *));

/*
 * Files
 *
 * The game reads and writes one file at a time through hf, a small handle
 * that Win16 OpenFile returned (file.c). These keep that interface on every
 * platform through the C++ file service, with UTF-8 native paths.
 * POSIX has no share modes, so a file another program has open is not refused.
 */

// HfOpenFile opens szFile with Win16 OpenFile flags (mdRead and the like)
// and returns its handle, or -1 with *pfMissing set if it does not exist.
int16_t HfOpenFile(const char *szFile, uint16_t mdOpen, int16_t *pfMissing);
int32_t CbReadFile(int16_t hf, void *rg, uint16_t cb);
int32_t CbWriteFile(int16_t hf, const void *rg, uint16_t cb);
// LSeekFile moves like _llseek: iOrigin 0 from the start, 1 from the
// current position, 2 from the end. It returns the new position.
int32_t LSeekFile(int16_t hf, int32_t l, int16_t iOrigin);
int32_t CbFileSize(int16_t hf);
void    CloseFile(int16_t hf);
int16_t FFileExists(const char *szFile);
int16_t FFileReadOnly(const char *szFile);
void    MakeDir(const char *szDir);
// Unlike MakeDir, reservation fails if the directory already exists.
int     CreateNativeDir(const char *pszDir);
using FNativeDirectoryEntryProc = int16_t (*)(APPSESSION &, void *lpData, const char *pszEntry);
int16_t FNativeEnumerateDirectory(APPSESSION &app, const char *pszDir, FNativeDirectoryEntryProc pfnEntry, void *lpData);
int16_t FNativeFileSize(const char *pszFile, int64_t *pcbFile);
int16_t FNativeDirectoryExists(const char *pszDir);
// Inspect the final component without following links or Windows reparse points.
int16_t FNativePathInfo(const char *psz, int16_t *pfDirectory, int16_t *pfLink);
int16_t FEnsureNativeDir(const char *pszDir);
int     RemoveNativeFile(const char *psz);
int     RemoveNativePath(const char *psz);
int     ReplaceNativeFile(const char *pszOld, const char *pszNew);
int64_t LNativeTime(void);
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
int16_t FSzPrefixNoCase(const char *sz1, const char *sz2, int16_t cch);

/*
 * Formatting and arithmetic
 *
 * Win32 long is 32 bits, so the game's formats (and its string table) pass
 * int32_t values to %ld. CchSprintf formats like wsprintf with l read as 32
 * bits on every platform. LMulDiv rounds like Win32 MulDiv.
 */
int     CchSprintf(char *sz, const char *szFormat, ...);
// Bounded native formatting uses standard C widths and reports truncation.
int     CchSnprintf(char *psz, size_t cb, const char *pszFormat, ...);
int32_t LMulDiv(int32_t lNumber, int32_t lNumerator, int32_t lDenominator);


#endif
