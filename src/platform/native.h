#ifndef STARS_NATIVE_H
#define STARS_NATIVE_H

// Helpers the Win32 build needs where Stars' Win16 types and behavior differ
// from Win32. See docs/NATIVE-PORT.md.

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include "encoding.h"

/*
 * Win32 basics
 *
 * The game code keeps the Win32 names it was written with. These define
 * them where windows.h is not included; the UI files include windows.h
 * first (win.h), and its definitions are the same.
 */

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif
#ifndef LOWORD
#define LOWORD(l)      ((uint16_t)(((uintptr_t)(l)) & 0xffff))
#define HIWORD(l)      ((uint16_t)((((uintptr_t)(l)) >> 16) & 0xffff))
#define LOBYTE(w)      ((uint8_t)(((uintptr_t)(w)) & 0xff))
#define MAKELONG(a, b) ((int32_t)(((uint16_t)(((uintptr_t)(a)) & 0xffff)) | ((uint32_t)((uint16_t)(((uintptr_t)(b)) & 0xffff))) << 16))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

// AlertSz's message box types and answers (IdAlertBox).
#ifndef MB_OK
#define MB_OK              0x0000
#define MB_OKCANCEL        0x0001
#define MB_YESNOCANCEL     0x0003
#define MB_YESNO           0x0004
#define MB_ICONHAND        0x0010
#define MB_ICONQUESTION    0x0020
#define MB_ICONEXCLAMATION 0x0030
#define MB_ICONASTERISK    0x0040
#define MB_TASKMODAL       0x2000
#define IDOK               1
#define IDCANCEL           2
#define IDYES              6
#define IDNO               7
#endif

// A Win32 RECT, for the frame window position in ini and the UI's structs.
#ifndef _WINDEF_
typedef struct tagRECT {
#ifdef _WIN32
    long left;
    long top;
    long right;
    long bottom;
#else
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
#endif
} RECT;
#endif

// Window and device context handles that game structs hold for the UI (RPT,
// TUTOR, TILE). These match windows.h's STRICT declarations.
typedef struct HWND__ *HWND;
typedef struct HDC__  *HDC;

/*
 * Win16 points
 *
 * Win16 POINT held 16-bit ints, and Stars writes records holding points to
 * its files. Stars' points are POINT16 to keep that layout; Win32 POINT holds
 * LONGs, so points convert where they pass into or out of the Win32 API
 * (nativeui.h).
 */

typedef struct tagPOINT16 {
    int16_t x;
    int16_t y;
} POINT16;

// qsort16 sorts like Stars' Win16 CRT qsort, which decides the order of
// elements with equal keys. Turn generation depends on that order.
void qsort16(void *base, size_t count, size_t width, int (*compare)(const void *, const void *));

/*
 * Files
 *
 * The game reads and writes one file at a time through hf, a small handle
 * that Win16 OpenFile returned (file.c). These keep that interface on every
 * platform: Windows converts UTF-8 paths to UTF-16 at the CRT boundary.
 * POSIX has no share modes, so a file another program has open is not refused.
 */

// HfOpenFile opens szFile with Win16 OpenFile flags (mdRead and the like)
// and returns its handle, or -1 with *pfMissing set if it does not exist.
int16_t HfOpenFile(char *szFile, uint16_t mdOpen, int16_t *pfMissing);
int32_t CbReadFile(int16_t hf, void *rg, uint16_t cb);
int32_t CbWriteFile(int16_t hf, void *rg, uint16_t cb);
// LSeekFile moves like _llseek: iOrigin 0 from the start, 1 from the
// current position, 2 from the end. It returns the new position.
int32_t LSeekFile(int16_t hf, int32_t l, int16_t iOrigin);
int32_t CbFileSize(int16_t hf);
void    CloseFile(int16_t hf);
int16_t FFileExists(char *szFile);
int16_t FFileReadOnly(char *szFile);
void    MakeDir(char *szDir);
// Unlike MakeDir, reservation fails if the directory already exists.
int     CreateNativeDir(const char *pszDir);
int     RemoveNativeFile(const char *psz);
int     ReplaceNativeFile(const char *pszOld, const char *pszNew);
int64_t LNativeTime(void);
int     RenameNativeFile(const char *pszOld, const char *pszNew);
FILE   *PfOpenNativeFile(const char *psz, const char *pszMode);
#ifdef _WIN32
char   *PszNativeCommandLine(void);
char  **RgpszNativeArguments(int *lpc);
#endif

// File names: the directory separator.
#ifdef _WIN32
#define chDirSep '\\'
#define szDirSep "\\"
#else
#define chDirSep '/'
#define szDirSep "/"
#endif

// DwTickCount counts milliseconds like GetTickCount, for retries and file
// salts.
uint32_t DwTickCount();
// GetDateTimeSz writes the date as MM/DD/YY and the time as HH:MM:SS, as
// _strdate and _strtime did, for the turn log.
void GetDateTimeSz(char *szDate, char *szTime);
// FSzPrefixNoCase compares the first cch characters, ignoring case.
int16_t FSzPrefixNoCase(char *sz1, char *sz2, int16_t cch);

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
