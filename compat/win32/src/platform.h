#ifndef STARS_COMPAT_PLATFORM_H
#define STARS_COMPAT_PLATFORM_H

#ifdef _WIN32
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <SDL3/SDL.h>

typedef struct COMPATDIR COMPATDIR;
struct COMPATDIRENT { char d_name[PATH_MAX]; };

char *PszCompatRealPath(const char *pszPath, char *pszOut);
int ICompatMakeDir(const char *pszPath, int md);
int ICompatRename(const char *pszOld, const char *pszNew);
int ICompatTempFile(char *pszTemplate);
ssize_t CbCompatGetLine(char **lpsz, size_t *lpcb, FILE *lpfile);
FILE *LpCompatOpenFile(const char *pszPath, const char *pszMode);
int ICompatUnlink(const char *pszPath);
int ICompatAccess(const char *pszPath, int md);
int ICompatOpen(const char *pszPath, int flags, ...);
int ICompatStat(const char *pszPath, struct stat *lpst);
char *PszCompatCwd(char *pszOut, size_t cb);
int ICompatChdir(const char *pszPath);
COMPATDIR *LpCompatOpenDir(const char *pszPath);
struct COMPATDIRENT *LpCompatReadDir(COMPATDIR *lpdir);
int ICompatCloseDir(COMPATDIR *lpdir);
int FCompatFileIdentity(int fd, const char *pszPath, uint64_t *lpdev, uint64_t *lpino);

#define realpath PszCompatRealPath
#define mkdir ICompatMakeDir
#define rename ICompatRename
#define mkstemp ICompatTempFile
#define getline CbCompatGetLine
#define fsync _commit
#define O_CLOEXEC _O_NOINHERIT
#define fopen LpCompatOpenFile
#define unlink ICompatUnlink
#define access ICompatAccess
#define open ICompatOpen
#define getcwd PszCompatCwd
#define chdir ICompatChdir
// Keep struct stat available while redirecting only function calls.
#define stat(psz, lpst) ICompatStat(psz, lpst)
#define DIR COMPATDIR
#define dirent COMPATDIRENT
#define opendir LpCompatOpenDir
#define readdir LpCompatReadDir
#define closedir ICompatCloseDir
#define getenv SDL_getenv
#endif

#endif
