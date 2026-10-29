#ifndef STARS_WINCOMPAT_DIRECT_H
#define STARS_WINCOMPAT_DIRECT_H
#include <stddef.h>
#ifdef _WIN32
#pragma GCC system_header
#include_next <direct.h>
#else
#ifdef __cplusplus
extern "C" {
#endif
int _mkdir(const char *pszPath);
char *_getcwd(char *pszPath, int cchPath);
#ifdef __cplusplus
}
#endif
#endif
#endif
