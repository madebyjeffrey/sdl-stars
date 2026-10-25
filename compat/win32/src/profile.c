/* File-backed UTF-8 profiles. Preserve unrelated lines when updating settings. */
#include <windows.h>
#include <ctype.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include "platform.h"
#include "encoding.h"

typedef struct COMPATPROFILE {
    char *szRaw, *szParsed, *pszSection, *pszKey, *pszValue;
    struct COMPATPROFILE *lpNext;
} COMPATPROFILE;

static atomic_flag fProfileLock = ATOMIC_FLAG_INIT;

static DWORD DwProfileError(void) {
    if (errno == ENOENT) return ERROR_FILE_NOT_FOUND;
    if (errno == EACCES || errno == EPERM) return ERROR_ACCESS_DENIED;
    if (errno == ENOMEM) return ERROR_NOT_ENOUGH_MEMORY;
    return ERROR_WRITE_FAULT;
}

static void FreeProfile(COMPATPROFILE *lp) {
    COMPATPROFILE *lpNext;
    while (lp) {
        lpNext = lp->lpNext;
        free(lp->szRaw); free(lp->szParsed); free(lp);
        lp = lpNext;
    }
}

static char *PszTrimProfile(char *psz) {
    char *pszEnd;
    while (*psz && isspace((unsigned char)*psz)) psz++;
    pszEnd = psz + strlen(psz);
    while (pszEnd > psz && isspace((unsigned char)pszEnd[-1])) *--pszEnd = 0;
    return psz;
}

static COMPATPROFILE *LpProfileLine(const char *pszRaw) {
    COMPATPROFILE *lp = calloc(1, sizeof(*lp));
    char *psz, *pszEnd;

    if (!lp) return NULL;
    lp->szRaw = strdup(pszRaw); lp->szParsed = strdup(pszRaw);
    if (!lp->szRaw || !lp->szParsed) { FreeProfile(lp); return NULL; }
    psz = PszTrimProfile(lp->szParsed);
    if (*psz == '[' && (pszEnd = strchr(psz + 1, ']'))) {
        *pszEnd = 0;
        lp->pszSection = PszTrimProfile(psz + 1);
    } else if (*psz && *psz != ';' && (pszEnd = strchr(psz, '='))) {
        *pszEnd = 0;
        lp->pszKey = PszTrimProfile(psz);
        lp->pszValue = PszTrimProfile(pszEnd + 1);
    }
    return lp;
}

static BOOL FLoadProfile(const char *pszPath, COMPATPROFILE **lplp) {
    FILE *fp;
    char *sz = NULL;
    size_t cb = 0, cbTotal = 0;
    ssize_t cbLine;
    COMPATPROFILE **lplpNext = lplp;
    BOOL fRet = FALSE;

    *lplp = NULL;
    fp = fopen(pszPath, "rb");
    if (!fp) {
        SetLastError(DwProfileError());
        return errno == ENOENT;
    }
    while ((cbLine = getline(&sz, &cb, fp)) >= 0) {
        cbTotal += (size_t)cbLine;
        if (cbTotal > 8 * 1024 * 1024 || !FUtf8Valid(sz, (size_t)cbLine)) {
            SetLastError(ERROR_BAD_FORMAT); goto LReturn;
        }
        if (cbTotal == (size_t)cbLine && cbLine >= 3 && !memcmp(sz, "\xef\xbb\xbf", 3))
            memmove(sz, sz + 3, (size_t)cbLine - 2);
        *lplpNext = LpProfileLine(sz);
        if (!*lplpNext) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); goto LReturn; }
        lplpNext = &(*lplpNext)->lpNext;
    }
    if (ferror(fp)) { SetLastError(ERROR_READ_FAULT); goto LReturn; }
    fRet = TRUE;
LReturn:
    free(sz); fclose(fp);
    if (!fRet) { FreeProfile(*lplp); *lplp = NULL; }
    return fRet;
}

static COMPATPROFILE *LpProfileSection(COMPATPROFILE *lp, LPCSTR pszSection) {
    if (!pszSection) return NULL;
    for (; lp; lp = lp->lpNext)
        if (lp->pszSection && !strcasecmp(lp->pszSection, pszSection)) return lp;
    return NULL;
}

static COMPATPROFILE *LpProfileKey(COMPATPROFILE *lp, LPCSTR pszKey) {
    if (!lp || !pszKey) return NULL;
    for (lp = lp->lpNext; lp && !lp->pszSection; lp = lp->lpNext)
        if (lp->pszKey && !strcasecmp(lp->pszKey, pszKey)) return lp;
    return NULL;
}

static const char *PszProfileValue(COMPATPROFILE *lp, size_t *pcch) {
    const char *psz = lp->pszValue;
    *pcch = strlen(psz);
    if (*pcch >= 2 && (psz[0] == '\'' || psz[0] == '"') && psz[*pcch - 1] == psz[0]) {
        psz++; *pcch -= 2;
    }
    return psz;
}

static char *SzProfilePath(LPCSTR pszFile) {
    const char *pszDir, *pszHome;
    char *sz;
    size_t cb;

    if (!pszFile || !*pszFile) { SetLastError(ERROR_INVALID_PARAMETER); return NULL; }
    // Explicit paths remain literal. Bare settings names use the user config directory.
    if (strchr(pszFile, '/')
#ifdef _WIN32
        || strchr(pszFile, '\\') || strchr(pszFile, ':')
#endif
    ) { sz = strdup(pszFile); goto LReturn; }
    pszDir = getenv("STARS_PROFILE_DIR");
    if (!pszDir || !*pszDir) pszDir = getenv("STARS_SETTINGS_DIR");
    if (pszDir && *pszDir) {
        cb = strlen(pszDir) + strlen(pszFile) + 2;
        sz = malloc(cb);
        if (sz) snprintf(sz, cb, "%s/%s", pszDir, pszFile);
        goto LReturn;
    }
    pszDir = getenv("XDG_CONFIG_HOME");
    pszHome = getenv("HOME");
#ifdef _WIN32
    pszDir = getenv("LOCALAPPDATA");
    if (pszDir && *pszDir) {
        cb = strlen(pszDir) + strlen(pszFile) + 8;
        sz = malloc(cb);
        if (sz) snprintf(sz, cb, "%s/stars/%s", pszDir, pszFile);
    } else sz = strdup(pszFile);
    (void)pszHome;
#else
    if (pszDir && *pszDir == '/') {
        cb = strlen(pszDir) + strlen(pszFile) + 8;
        sz = malloc(cb);
        if (sz) snprintf(sz, cb, "%s/stars/%s", pszDir, pszFile);
    } else if (pszHome && *pszHome == '/') {
        cb = strlen(pszHome) + strlen(pszFile) + 16;
        sz = malloc(cb);
        if (sz) snprintf(sz, cb, "%s/.config/stars/%s", pszHome, pszFile);
    } else {
        sz = strdup(pszFile);
    }
#endif
LReturn:
    if (!sz) SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    else if (!FUtf8Valid(sz, strlen(sz))) { free(sz); sz = NULL; SetLastError(ERROR_INVALID_PARAMETER); }
    return sz;
}

static BOOL FMakeProfileDir(char *szPath) {
    char *psz;
    char ch;
    int err;

    for (psz = szPath + 1; *psz; psz++) {
        if (*psz != '/'
#ifdef _WIN32
            && *psz != '\\'
#endif
        ) continue;
#ifdef _WIN32
        if (psz == szPath + 2 && szPath[1] == ':') continue;
#endif
        ch = *psz;
        *psz = 0;
        err = mkdir(szPath, 0700);
        *psz = ch;
        if (err != 0 && errno != EEXIST) { SetLastError(DwProfileError()); return FALSE; }
    }
    return TRUE;
}

DWORD WINAPI GetPrivateProfileStringA(LPCSTR pszSection, LPCSTR pszKey, LPCSTR pszDefault,
                                    LPSTR szResult, DWORD cchMax, LPCSTR pszFile) {
    char *szPath;
    COMPATPROFILE *lpAll = NULL, *lp, *lpKey;
    const char *psz;
    size_t cch, cchOut = 0, cchCopy;
    BOOL fMulti = !pszSection || !pszKey;

    if (!szResult || cchMax == 0) return 0;
    szResult[0] = 0;
    if (fMulti && cchMax > 1) szResult[1] = 0;
    szPath = SzProfilePath(pszFile);
    if (!szPath) return 0;
    while (atomic_flag_test_and_set_explicit(&fProfileLock, memory_order_acquire)) {}
    FLoadProfile(szPath, &lpAll);
    if (fMulti) {
        lp = pszSection ? LpProfileSection(lpAll, pszSection) : lpAll;
        if (pszSection && lp) lp = lp->lpNext;
        for (; lp; lp = lp->lpNext) {
            if (pszSection && lp->pszSection) break;
            psz = pszSection ? lp->pszKey : lp->pszSection;
            if (!psz || !*psz) continue;
            cch = strlen(psz);
            if (cchMax < 2) break;
            if (cchOut > cchMax - 2) { cchOut = cchMax - 2; break; }
            cchCopy = CchUtf8Prefix(psz, cch < cchMax - 2 - cchOut ? cch : cchMax - 2 - cchOut);
            memcpy(szResult + cchOut, psz, cchCopy);
            cchOut += cchCopy;
            if (cchCopy < cch || cchOut + 1 >= cchMax) {
                memset(szResult + cchOut, 0, cchMax - cchOut);
                cchOut = cchMax - 2; break;
            }
            szResult[cchOut++] = 0;
            szResult[cchOut] = 0;
        }
        // The reference API reports nSize-2 even when a list exactly fills the buffer.
        if (cchMax >= 2 && cchOut == cchMax - 1) cchOut = cchMax - 2;
    } else {
        lpKey = LpProfileKey(LpProfileSection(lpAll, pszSection), pszKey);
        if (lpKey) psz = PszProfileValue(lpKey, &cch);
        else {
            psz = pszDefault ? pszDefault : "";
            cch = strlen(psz);
            while (cch != 0 && psz[cch - 1] == ' ') cch--;
        }
        cchOut = CchUtf8Prefix(psz, cch < cchMax - 1 ? cch : cchMax - 1);
        memcpy(szResult, psz, cchOut); szResult[cchOut] = 0;
    }
    FreeProfile(lpAll); free(szPath);
    atomic_flag_clear_explicit(&fProfileLock, memory_order_release);
    return (DWORD)cchOut;
}

UINT WINAPI GetPrivateProfileIntA(LPCSTR pszSection, LPCSTR pszKey, INT nDefault, LPCSTR pszFile) {
    char sz[128], *psz = sz;
    UINT u = 0, digit, radix = 10;
    BOOL fNegative = FALSE;

    if (GetPrivateProfileStringA(pszSection, pszKey, "", sz, sizeof(sz), pszFile) == 0)
        return (UINT)nDefault;
    while (isspace((unsigned char)*psz)) psz++;
    if (*psz == '+' || *psz == '-') { fNegative = *psz == '-'; psz++; }
    if (psz[0] == '0' && (psz[1] == 'x' || psz[1] == 'X')) { radix = 16; psz += 2; }
    for (; *psz; psz++) {
        if (*psz >= '0' && *psz <= '9') digit = (UINT)(*psz - '0');
        else if (*psz >= 'a' && *psz <= 'f') digit = (UINT)(*psz - 'a' + 10);
        else if (*psz >= 'A' && *psz <= 'F') digit = (UINT)(*psz - 'A' + 10);
        else break;
        if (digit >= radix) break;
        u = u * radix + digit;
    }
    return fNegative ? 0u - u : u;
}

static BOOL FSaveProfile(COMPATPROFILE *lp, const char *pszPath) {
    char *szTemp;
    FILE *fp;
    int fd;
    BOOL fRet = FALSE;
#ifndef _WIN32
    struct stat st;
#endif

    szTemp = malloc(strlen(pszPath) + 12);
    if (!szTemp) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return FALSE; }
    sprintf(szTemp, "%s.XXXXXX", pszPath);
    fd = mkstemp(szTemp);
    if (fd < 0) { SetLastError(DwProfileError()); goto LReturn; }
#ifndef _WIN32
    if (stat(pszPath, &st) == 0 && fchmod(fd, st.st_mode & 0777) != 0) {
        SetLastError(DwProfileError()); close(fd); goto LReturn;
    }
#endif
    fp = fdopen(fd, "wb");
    if (!fp) { SetLastError(DwProfileError()); close(fd); goto LReturn; }
    if (fwrite("\xef\xbb\xbf", 1, 3, fp) != 3) { fclose(fp); goto LReturn; }
    for (; lp; lp = lp->lpNext) {
        if (fputs(lp->szRaw, fp) == EOF) break;
        if (lp->lpNext && *lp->szRaw && lp->szRaw[strlen(lp->szRaw) - 1] != '\n')
            if (fputc('\n', fp) == EOF) break;
    }
    if (lp || fflush(fp) != 0 || fsync(fd) != 0) {
        SetLastError(DwProfileError()); fclose(fp); goto LReturn;
    }
    if (fclose(fp) != 0 || rename(szTemp, pszPath) != 0) { SetLastError(DwProfileError()); goto LReturn; }
    fRet = TRUE;
LReturn:
    if (!fRet) unlink(szTemp);
    free(szTemp);
    return fRet;
}

BOOL WINAPI WritePrivateProfileStringA(LPCSTR pszSection, LPCSTR pszKey, LPCSTR pszValue, LPCSTR pszFile) {
    char *szPath, *szLine = NULL;
    COMPATPROFILE *lpAll = NULL, *lpSection, *lpKey, *lpNew, *lp;
    COMPATPROFILE **lplp;
    BOOL fRet = FALSE;
    size_t cb;

    if (!pszSection && !pszKey && !pszValue) return FALSE; // No cache to flush.
    if (!pszSection || !FUtf8Valid(pszSection, strlen(pszSection)) ||
        (pszKey && !FUtf8Valid(pszKey, strlen(pszKey))) ||
        (pszValue && !FUtf8Valid(pszValue, strlen(pszValue))) || !*pszSection || strchr(pszSection, '\n') || strchr(pszSection, '\r') ||
        strchr(pszSection, ']') || (pszKey && (strchr(pszKey, '=') || strchr(pszKey, '\n') || strchr(pszKey, '\r'))) ||
        (pszValue && (strchr(pszValue, '\n') || strchr(pszValue, '\r')))) {
        SetLastError(ERROR_INVALID_PARAMETER); return FALSE;
    }
    szPath = SzProfilePath(pszFile);
    if (!szPath) return FALSE;
    while (atomic_flag_test_and_set_explicit(&fProfileLock, memory_order_acquire)) {}
    if (!FLoadProfile(szPath, &lpAll)) goto LReturn;
    lpSection = LpProfileSection(lpAll, pszSection);
    lpKey = LpProfileKey(lpSection, pszKey);
    if (!pszKey || !pszValue) {
        if (!lpSection || (pszKey && !lpKey)) { fRet = TRUE; goto LReturn; }
        for (lplp = &lpAll; *lplp && *lplp != (pszKey ? lpKey : lpSection); lplp = &(*lplp)->lpNext) {}
        do {
            lp = *lplp; *lplp = lp->lpNext; lp->lpNext = NULL; FreeProfile(lp);
        } while (!pszKey && *lplp && !(*lplp)->pszSection);
    } else {
        if (!lpSection) {
            cb = strlen(pszSection) + 4;
            szLine = malloc(cb);
            if (!szLine) goto LMemory;
            snprintf(szLine, cb, "[%s]\n", pszSection);
            lpSection = LpProfileLine(szLine);
            free(szLine); szLine = NULL;
            if (!lpSection) goto LMemory;
            for (lplp = &lpAll; *lplp; lplp = &(*lplp)->lpNext) {}
            *lplp = lpSection;
        }
        cb = strlen(pszKey) + strlen(pszValue) + 3;
        szLine = malloc(cb);
        if (!szLine) goto LMemory;
        snprintf(szLine, cb, "%s=%s\n", pszKey, pszValue);
        lpNew = LpProfileLine(szLine);
        if (!lpNew) goto LMemory;
        for (lplp = &lpSection->lpNext; *lplp && !(*lplp)->pszSection && *lplp != lpKey; lplp = &(*lplp)->lpNext) {}
        if (lpKey) {
            lpNew->lpNext = lpKey->lpNext; lpKey->lpNext = NULL; FreeProfile(lpKey);
        } else lpNew->lpNext = *lplp;
        *lplp = lpNew;
    }
    if (!strchr(pszFile, '/') && !FMakeProfileDir(szPath)) goto LReturn;
    fRet = FSaveProfile(lpAll, szPath);
    goto LReturn;
LMemory:
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
LReturn:
    free(szLine); free(szPath); FreeProfile(lpAll);
    atomic_flag_clear_explicit(&fProfileLock, memory_order_release);
    return fRet;
}
