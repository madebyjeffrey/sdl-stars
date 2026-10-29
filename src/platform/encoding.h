#ifndef STARS_ENCODING_H
#define STARS_ENCODING_H
#include <stddef.h>
#include <stdint.h>
// All decoded application text and supported paths are UTF-8.
// Strict copies/conversions return bytes written, -1 malformed or -3 capacity.
// A failed operation leaves an empty destination.
int FUtf8Valid(const char *psz, size_t cb);
int CchCopyUtf8(char *pszOut, size_t cbOut, const char *psz, size_t cb);
int CchCp1252ToUtf8(char *pszOut, size_t cbOut, const unsigned char *rgb, size_t cb);
int CchUtf16leToUtf8(char *pszOut, size_t cbOut, const unsigned char *rgb, size_t cch);
int CbUtf8Char(const char *psz, size_t cb, uint32_t *lpch);
size_t CchUtf8Prefix(const char *psz, size_t cchMax);

#endif
