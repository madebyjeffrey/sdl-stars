/* Legacy HFILE adapters use owned C++ file tokens, never arbitrary integer fds. */
#include "internal.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct COMPATFILE {
    HFILE hf;
    NATIVEFILE *pf;
    UINT grAccess, grShare;
    uint64_t dev, ino;
    struct COMPATFILE *lpNext;
} COMPATFILE;

static COMPATFILE *lpFiles;
static atomic_flag fFileLock = ATOMIC_FLAG_INIT;

static void LockFiles(void) {
    while (atomic_flag_test_and_set_explicit(&fFileLock, memory_order_acquire)) {}
}
static void UnlockFiles(void) { atomic_flag_clear_explicit(&fFileLock, memory_order_release); }
static COMPATFILE *LpFile(HFILE hf) {
    COMPATFILE *lp;
    for (lp = lpFiles; lp; lp = lp->lpNext) if (lp->hf == hf) return lp;
    SetLastError(ERROR_INVALID_HANDLE);
    return NULL;
}
static DWORD DwFileError(void) {
    switch (errno) {
    case ENOENT: return ERROR_FILE_NOT_FOUND;
    case ENOTDIR: return ERROR_PATH_NOT_FOUND;
    case EACCES: case EPERM: case EISDIR: return ERROR_ACCESS_DENIED;
    case EMFILE: case ENFILE: return ERROR_TOO_MANY_OPEN_FILES;
    case ENOMEM: return ERROR_NOT_ENOUGH_MEMORY;
    case ENOSPC: return ERROR_DISK_FULL;
    case EBADF: return ERROR_INVALID_HANDLE;
    case EINVAL: return ERROR_INVALID_PARAMETER;
    default: return ERROR_READ_FAULT;
    }
}

HFILE WINAPI OpenFile(LPCSTR pszFile, LPOFSTRUCT lpof, UINT flags) {
    char szPath[PATH_MAX], szCwd[PATH_MAX];
    COMPATFILE *lp, *lpNew = NULL;
    NATIVEFILEINFO st;
    UINT grAccess, grShare, share = flags & 0x70;
    NATIVEFILE *pf = NULL;
    HFILE hf = HFILE_ERROR;
    DWORD dwError = NO_ERROR;
    uint64_t dev, ino;

    if (!lpof) { SetLastError(ERROR_INVALID_PARAMETER); return HFILE_ERROR; }
    if (flags & OF_REOPEN) pszFile = lpof->szPathName;
    if (!pszFile || !*pszFile || !FUtf8Valid(pszFile, strlen(pszFile)) || (flags & 3) == 3 ||
        (share != OF_SHARE_COMPAT && share != OF_SHARE_EXCLUSIVE && share != OF_SHARE_DENY_WRITE &&
         share != OF_SHARE_DENY_READ && share != OF_SHARE_DENY_NONE)) {
        dwError = ERROR_INVALID_PARAMETER; goto LReturn;
    }
    if (flags & ~(3u | 0x70u | OF_PARSE | OF_DELETE | OF_CREATE | OF_EXIST | OF_REOPEN | OF_CANCEL)) {
        dwError = ERROR_NOT_SUPPORTED; goto LReturn;
    }
    if (*pszFile == '/'
#ifdef _WIN32
        || *pszFile == '\\' || (pszFile[0] && pszFile[1] == ':')
#endif
    ) {
        if (strlen(pszFile) >= sizeof(szPath)) { dwError = ERROR_FILENAME_EXCED_RANGE; goto LReturn; }
        strcpy(szPath, pszFile);
    } else {
        if (!PszNativeCwd(szCwd, sizeof(szCwd))) { dwError = DwFileError(); goto LReturn; }
        if (snprintf(szPath, sizeof(szPath), "%s/%s", szCwd, pszFile) >= (int)sizeof(szPath)) {
            dwError = ERROR_FILENAME_EXCED_RANGE; goto LReturn;
        }
    }
    if (!FUtf8Valid(szPath, strlen(szPath))) { dwError = ERROR_INVALID_PARAMETER; goto LReturn; }
    if (strlen(szPath) >= sizeof(lpof->szPathName)) { dwError = ERROR_FILENAME_EXCED_RANGE; goto LReturn; }
    memset(lpof, 0, sizeof(*lpof));
    lpof->cBytes = sizeof(*lpof); lpof->fFixedDisk = TRUE;
    strcpy(lpof->szPathName, szPath);
    if (flags & OF_PARSE) return 0;
    grAccess = flags & OF_CREATE ? 3 : (flags & 3) == OF_READ ? 1 : (flags & 3) == OF_WRITE ? 2 : 3;
    grShare = share == OF_SHARE_EXCLUSIVE ? 0 : share == OF_SHARE_DENY_WRITE ? 1 : share == OF_SHARE_DENY_READ ? 2 : 3;
    LockFiles();
    if (flags & OF_DELETE) {
        if (IGetNativeFileInfo(szPath, &st, TRUE) == 0) {
            if (!FNativeFileIdentity(NULL, szPath, &dev, &ino)) { dwError = ERROR_ACCESS_DENIED; goto LUnlock; }
            for (lp = lpFiles; lp; lp = lp->lpNext)
                if (lp->dev == dev && lp->ino == ino) { dwError = ERROR_SHARING_VIOLATION; goto LUnlock; }
        }
        if (IUnlinkNativePath(szPath) != 0) dwError = DwFileError();
        else hf = 1;
        goto LUnlock;
    }
    // Check sharing before truncating an existing file.
    pf = PfNativeOpenFlags(szPath, grAccess, 0, (flags & OF_CREATE) != 0, FALSE, FALSE, TRUE);
    if (!pf || IGetNativeOpenInfo(pf, &st) != 0) { dwError = DwFileError(); goto LUnlock; }
    if (!st.fRegular) { dwError = ERROR_ACCESS_DENIED; goto LUnlock; }
    if (!FNativeFileIdentity(pf, NULL, &dev, &ino)) { dwError = ERROR_ACCESS_DENIED; goto LUnlock; }
    for (lp = lpFiles; lp; lp = lp->lpNext)
        if (lp->dev == dev && lp->ino == ino &&
            ((grAccess & ~lp->grShare) || (lp->grAccess & ~grShare))) {
            dwError = ERROR_SHARING_VIOLATION; goto LUnlock;
        }
    if ((flags & OF_CREATE) && ITruncateNativeFile(pf, 0) != 0) { dwError = DwFileError(); goto LUnlock; }
    if (flags & OF_EXIST) { hf = 1; goto LUnlock; }
    lpNew = calloc(1, sizeof(*lpNew));
    if (!lpNew) { dwError = ERROR_NOT_ENOUGH_MEMORY; goto LUnlock; }
    for (hf = 3; hf <= INT16_MAX && LpFile(hf); hf++) {}
    if (hf > INT16_MAX) { hf = HFILE_ERROR; dwError = ERROR_TOO_MANY_OPEN_FILES; goto LUnlock; }
    lpNew->hf = hf; lpNew->pf = pf; lpNew->dev = dev; lpNew->ino = ino;
    lpNew->grAccess = grAccess; lpNew->grShare = grShare;
    lpNew->lpNext = lpFiles; lpFiles = lpNew; lpNew = NULL; pf = NULL;
LUnlock:
    if (pf) ICloseNativeFile(pf);
    free(lpNew);
    UnlockFiles();
LReturn:
    lpof->nErrCode = (WORD)dwError;
    if (dwError != NO_ERROR) SetLastError(dwError);
    return hf;
}

HFILE WINAPI _lclose(HFILE hf) {
    COMPATFILE **lplp, *lp;
    HFILE ret = HFILE_ERROR;
    BOOL fFound = FALSE;

    LockFiles();
    for (lplp = &lpFiles; *lplp; lplp = &(*lplp)->lpNext) {
        lp = *lplp;
        if (lp->hf != hf) continue;
        fFound = TRUE;
        *lplp = lp->lpNext;
        if (ICloseNativeFile(lp->pf) == 0) ret = 0;
        else SetLastError(DwFileError());
        free(lp); break;
    }
    if (!fFound) SetLastError(ERROR_INVALID_HANDLE);
    UnlockFiles();
    return ret;
}

LONG WINAPI _llseek(HFILE hf, LONG offset, int origin) {
    COMPATFILE *lp;
    int64_t pos = -1;

    LockFiles(); lp = LpFile(hf);
    if (lp) {
        if (origin < 0 || origin > 2) SetLastError(ERROR_INVALID_PARAMETER);
        else {
            pos = LSeekNativeFile(lp->pf, offset, origin);
            if (pos < 0) SetLastError(DwFileError());
        }
    }
    UnlockFiles();
    return (LONG)pos;
}

UINT WINAPI _lread(HFILE hf, LPVOID rgb, UINT cb) {
    COMPATFILE *lp;
    ssize_t cbRead = -1;

    LockFiles(); lp = LpFile(hf);
    if (lp) {
        if (!(lp->grAccess & 1)) SetLastError(ERROR_ACCESS_DENIED);
        else if (!rgb && cb != 0) SetLastError(ERROR_INVALID_PARAMETER);
        else {
            do { cbRead = CbReadNativeFile(lp->pf, rgb, cb); } while (cbRead < 0 && errno == EINTR);
            if (cbRead < 0) SetLastError(DwFileError());
        }
    }
    UnlockFiles();
    return (UINT)cbRead;
}

UINT WINAPI _lwrite(HFILE hf, LPCCH rgb, UINT cb) {
    COMPATFILE *lp;
    ssize_t cbWrite = -1;
    int64_t pos;

    LockFiles(); lp = LpFile(hf);
    if (lp) {
        if (!(lp->grAccess & 2)) SetLastError(ERROR_ACCESS_DENIED);
        else if (!rgb && cb != 0) SetLastError(ERROR_INVALID_PARAMETER);
        else if (cb == 0) {
            pos = LNativePosition(lp->pf);
            if (pos >= 0 && ITruncateNativeFile(lp->pf, pos) == 0) cbWrite = 0;
            else SetLastError(DwFileError());
        } else {
            do { cbWrite = CbWriteNativeFile(lp->pf, rgb, cb); } while (cbWrite < 0 && errno == EINTR);
            if (cbWrite < 0) SetLastError(DwFileError());
        }
    }
    UnlockFiles();
    return (UINT)cbWrite;
}

DWORD WINAPI GetFileSize(HANDLE h, LPDWORD lpdwHigh) {
    COMPATFILE *lp;
    int64_t cb;
    DWORD dw = INVALID_FILE_SIZE;
    INT_PTR id = (INT_PTR)h;

    LockFiles();
    lp = id >= 0 && id <= INT_MAX ? LpFile((HFILE)id) : NULL;
    if (lp && (cb = CbNativeFile(lp->pf)) >= 0) {
        dw = (DWORD)(uint64_t)cb;
        if (lpdwHigh) *lpdwHigh = (DWORD)((uint64_t)cb >> 32);
        SetLastError(NO_ERROR);
    } else if (lp) SetLastError(DwFileError());
    else SetLastError(ERROR_INVALID_HANDLE);
    UnlockFiles();
    return dw;
}

void CompatShutdownFiles(void) {
    COMPATFILE *lp;
    LockFiles();
    while (lpFiles) {
        lp = lpFiles; lpFiles = lp->lpNext;
        ICloseNativeFile(lp->pf); free(lp);
    }
    UnlockFiles();
}
