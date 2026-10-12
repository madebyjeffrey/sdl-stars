#ifndef STARS_ENCODING_H
#define STARS_ENCODING_H
#include <stddef.h>
#include <stdint.h>

// Native paths and command lines are UTF-8; saved game text stays CP1252.
// Strict conversions return bytes written, or -1 malformed, -2 unrepresentable,
// -3 insufficient capacity. A failed conversion leaves an empty destination.
int CchUtf8ToLegacy(char *pszOut, size_t cbOut, const char *psz, size_t cb);
int CchLegacyToUtf8(char *pszOut, size_t cbOut, const char *psz, size_t cb);
int CbUtf8Char(const char *psz, size_t cb, uint32_t *lpch);
size_t CchUtf8Prefix(const char *psz, size_t cchMax);
#endif
