#ifndef STARS_WINCOMPAT_DIRECT_H
#define STARS_WINCOMPAT_DIRECT_H
#include <stddef.h>
int _mkdir(const char *pszPath);
char *_getcwd(char *pszPath, int cchPath);
#endif
