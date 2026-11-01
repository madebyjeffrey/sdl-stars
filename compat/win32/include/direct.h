#ifndef STARS_WINCOMPAT_DIRECT_H
#define STARS_WINCOMPAT_DIRECT_H
#include <cstddef>
#ifdef _WIN32
#pragma GCC system_header
#include_next <direct.h>
#else
int _mkdir(const char *pszPath);
char *_getcwd(char *pszPath, int cchPath);

#endif
#endif
