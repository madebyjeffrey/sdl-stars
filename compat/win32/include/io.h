#ifndef STARS_WINCOMPAT_IO_H
#define STARS_WINCOMPAT_IO_H
#include <stddef.h>
#ifdef _WIN32
#pragma GCC system_header
#include_next <io.h>
#else
int _access(const char *pszPath, int mdAccess);
int _stricmp(const char *psz1, const char *psz2);
int _strnicmp(const char *psz1, const char *psz2, size_t cb);
int strcmpi(const char *psz1, const char *psz2);
int strnicmp(const char *psz1, const char *psz2, size_t cb);
char *_strdate(char *szDate);
char *_strtime(char *szTime);
char *_strlwr(char *sz);
char *strlwr(char *sz);
char *itoa(int value, char *sz, int radix);
#endif
#endif
