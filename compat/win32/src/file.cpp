#include "locking.h"
#include "file_read.h"
/* Legacy HFILE adapters use owned C++ file tokens, never arbitrary integer fds. */
#include "internal.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static thread_local decltype(&CbReadNativeFile) pfnReadFile = CbReadNativeFile;
COMPATFILEREADSCOPE::COMPATFILEREADSCOPE(decltype(&CbReadNativeFile) pfnRead) noexcept : pfnPrev(pfnReadFile) { pfnReadFile = pfnRead; }
COMPATFILEREADSCOPE::~COMPATFILEREADSCOPE() noexcept { pfnReadFile = pfnPrev; }

typedef struct COMPATFILE {
    HFILE hf;
    NATIVEFILE *pf;
    UINT grAccess, grShare;
    uint64_t dev, ino;
    struct COMPATFILE *lpNext;
} COMPATFILE;

static COMPATFILE *lpFiles;
static NATIVEFLAGLOCK fFileLock;

static COMPATFILE *LpFile(HFILE hf) {
    COMPATFILE *lp;
    for (lp = lpFiles; lp; lp = lp->lpNext) if (lp->hf == hf) return lp;
    win32::SetLastError(win32::ERROR_INVALID_HANDLE);
    return NULL;
}
static DWORD DwFileError(void) {
    switch (errno) {
    case ENOENT: return win32::ERROR_FILE_NOT_FOUND;
    case ENOTDIR: return win32::ERROR_PATH_NOT_FOUND;
    case EACCES: case EPERM: case EISDIR: return win32::ERROR_ACCESS_DENIED;
    case EMFILE: case ENFILE: return win32::ERROR_TOO_MANY_OPEN_FILES;
    case ENOMEM: return win32::ERROR_NOT_ENOUGH_MEMORY;
    case ENOSPC: return win32::ERROR_DISK_FULL;
    case EBADF: return win32::ERROR_INVALID_HANDLE;
    case EINVAL: return win32::ERROR_INVALID_PARAMETER;
    default: return win32::ERROR_READ_FAULT;
    }
}

HFILE win32::OpenFile(LPCSTR pszFile, LPOFSTRUCT lpof, UINT flags) {
    char szPath[PATH_MAX], szCwd[PATH_MAX];
    COMPATFILE *lp, *lpNew = NULL;
    NATIVEFILEINFO st;
    UINT grAccess, grShare, share = flags & 0x70;
    NATIVEFILE *pf = NULL;
    HFILE hf = win32::HFILE_ERROR;
    DWORD dwError = win32::NO_ERROR;
    uint64_t dev, ino;
    std::unique_lock lockFiles(fFileLock, std::defer_lock);

    if (!lpof) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return win32::HFILE_ERROR; }
    if (flags & win32::OF_REOPEN) pszFile = lpof->szPathName;
    if (!pszFile || !*pszFile || !FUtf8Valid(pszFile, strlen(pszFile)) || (flags & 3) == 3 ||
        (share != win32::OF_SHARE_COMPAT && share != win32::OF_SHARE_EXCLUSIVE && share != win32::OF_SHARE_DENY_WRITE &&
         share != win32::OF_SHARE_DENY_READ && share != win32::OF_SHARE_DENY_NONE)) {
        dwError = win32::ERROR_INVALID_PARAMETER; goto LReturn;
    }
    if (flags & ~(3u | 0x70u | win32::OF_PARSE | win32::OF_DELETE | win32::OF_CREATE | win32::OF_EXIST | win32::OF_REOPEN | win32::OF_CANCEL)) {
        dwError = win32::ERROR_NOT_SUPPORTED; goto LReturn;
    }
    if (*pszFile == '/'
#ifdef _WIN32
        || *pszFile == '\\' || (pszFile[0] && pszFile[1] == ':')
#endif
    ) {
        if (strlen(pszFile) >= sizeof(szPath)) { dwError = win32::ERROR_FILENAME_EXCED_RANGE; goto LReturn; }
        strcpy(szPath, pszFile);
    } else {
        if (!PszNativeCwd(szCwd, sizeof(szCwd))) { dwError = DwFileError(); goto LReturn; }
        if (snprintf(szPath, sizeof(szPath), "%s/%s", szCwd, pszFile) >= (int)sizeof(szPath)) {
            dwError = win32::ERROR_FILENAME_EXCED_RANGE; goto LReturn;
        }
    }
    if (!FUtf8Valid(szPath, strlen(szPath))) { dwError = win32::ERROR_INVALID_PARAMETER; goto LReturn; }
    if (strlen(szPath) >= sizeof(lpof->szPathName)) { dwError = win32::ERROR_FILENAME_EXCED_RANGE; goto LReturn; }
    memset(lpof, 0, sizeof(*lpof));
    lpof->cBytes = sizeof(*lpof); lpof->fFixedDisk = 1;
    strcpy(lpof->szPathName, szPath);
    if (flags & win32::OF_PARSE) return 0;
    grAccess = flags & win32::OF_CREATE ? 3 : (flags & 3) == win32::OF_READ ? 1 : (flags & 3) == win32::OF_WRITE ? 2 : 3;
    grShare = share == win32::OF_SHARE_EXCLUSIVE ? 0 : share == win32::OF_SHARE_DENY_WRITE ? 1 : share == win32::OF_SHARE_DENY_READ ? 2 : 3;
    lockFiles.lock();
    if (flags & win32::OF_DELETE) {
        if (IGetNativeFileInfo(szPath, &st, 1) == 0) {
            if (!FNativeFileIdentity(NULL, szPath, &dev, &ino)) { dwError = win32::ERROR_ACCESS_DENIED; goto LUnlock; }
            for (lp = lpFiles; lp; lp = lp->lpNext)
                if (lp->dev == dev && lp->ino == ino) { dwError = win32::ERROR_SHARING_VIOLATION; goto LUnlock; }
        }
        if (IUnlinkNativePath(szPath) != 0) dwError = DwFileError();
        else hf = 1;
        goto LUnlock;
    }
    // Check sharing before truncating an existing file.
    pf = PfNativeOpenFlags(szPath, grAccess, 0, (flags & win32::OF_CREATE) != 0, 0, 0, 1);
    if (!pf || IGetNativeOpenInfo(pf, &st) != 0) { dwError = DwFileError(); goto LUnlock; }
    if (!st.fRegular) { dwError = win32::ERROR_ACCESS_DENIED; goto LUnlock; }
    if (!FNativeFileIdentity(pf, NULL, &dev, &ino)) { dwError = win32::ERROR_ACCESS_DENIED; goto LUnlock; }
    for (lp = lpFiles; lp; lp = lp->lpNext)
        if (lp->dev == dev && lp->ino == ino &&
            ((grAccess & ~lp->grShare) || (lp->grAccess & ~grShare))) {
            dwError = win32::ERROR_SHARING_VIOLATION; goto LUnlock;
        }
    if ((flags & win32::OF_CREATE) && ITruncateNativeFile(pf, 0) != 0) { dwError = DwFileError(); goto LUnlock; }
    if (flags & win32::OF_EXIST) { hf = 1; goto LUnlock; }
    lpNew = (COMPATFILE *)(calloc(1, sizeof(*lpNew)));
    if (!lpNew) { dwError = win32::ERROR_NOT_ENOUGH_MEMORY; goto LUnlock; }
    for (hf = 3; hf <= INT16_MAX && LpFile(hf); hf++) {}
    if (hf > INT16_MAX) { hf = win32::HFILE_ERROR; dwError = win32::ERROR_TOO_MANY_OPEN_FILES; goto LUnlock; }
    lpNew->hf = hf; lpNew->pf = pf; lpNew->dev = dev; lpNew->ino = ino;
    lpNew->grAccess = grAccess; lpNew->grShare = grShare;
    lpNew->lpNext = lpFiles; lpFiles = lpNew; lpNew = NULL; pf = NULL;
LUnlock:
    if (pf) ICloseNativeFile(pf);
    free(lpNew);
    lockFiles.unlock();
LReturn:
    lpof->nErrCode = (WORD)dwError;
    if (dwError != win32::NO_ERROR) win32::SetLastError(dwError);
    return hf;
}

HFILE win32::_lclose(HFILE hf) {
    COMPATFILE **lplp, *lp;
    HFILE ret = win32::HFILE_ERROR;
    BOOL fFound = 0;

    std::unique_lock lockFiles(fFileLock);
    for (lplp = &lpFiles; *lplp; lplp = &(*lplp)->lpNext) {
        lp = *lplp;
        if (lp->hf != hf) continue;
        fFound = 1;
        *lplp = lp->lpNext;
        if (ICloseNativeFile(lp->pf) == 0) ret = 0;
        else win32::SetLastError(DwFileError());
        free(lp); break;
    }
    if (!fFound) win32::SetLastError(win32::ERROR_INVALID_HANDLE);
    lockFiles.unlock();
    return ret;
}

LONG win32::_llseek(HFILE hf, LONG offset, int origin) {
    COMPATFILE *lp;
    int64_t pos = -1;

    std::unique_lock lockFiles(fFileLock); lp = LpFile(hf);
    if (lp) {
        if (origin < 0 || origin > 2) win32::SetLastError(win32::ERROR_INVALID_PARAMETER);
        else {
            pos = LSeekNativeFile(lp->pf, offset, origin);
            if (pos < 0) win32::SetLastError(DwFileError());
        }
    }
    lockFiles.unlock();
    return (LONG)pos;
}

UINT win32::_lread(HFILE hf, LPVOID rgb, UINT cb) {
    COMPATFILE *lp;
    ssize_t cbRead = -1;

    std::unique_lock lockFiles(fFileLock); lp = LpFile(hf);
    if (lp) {
        if (!(lp->grAccess & 1)) win32::SetLastError(win32::ERROR_ACCESS_DENIED);
        else if (!rgb && cb != 0) win32::SetLastError(win32::ERROR_INVALID_PARAMETER);
        else {
            do { cbRead = pfnReadFile(lp->pf, rgb, cb); } while (cbRead < 0 && errno == EINTR);
            if (cbRead < 0) win32::SetLastError(DwFileError());
        }
    }
    lockFiles.unlock();
    return (UINT)cbRead;
}

UINT win32::_lwrite(HFILE hf, LPCCH rgb, UINT cb) {
    COMPATFILE *lp;
    ssize_t cbWrite = -1;
    int64_t pos;

    std::unique_lock lockFiles(fFileLock); lp = LpFile(hf);
    if (lp) {
        if (!(lp->grAccess & 2)) win32::SetLastError(win32::ERROR_ACCESS_DENIED);
        else if (!rgb && cb != 0) win32::SetLastError(win32::ERROR_INVALID_PARAMETER);
        else if (cb == 0) {
            pos = LNativePosition(lp->pf);
            if (pos >= 0 && ITruncateNativeFile(lp->pf, pos) == 0) cbWrite = 0;
            else win32::SetLastError(DwFileError());
        } else {
            do { cbWrite = CbWriteNativeFile(lp->pf, rgb, cb); } while (cbWrite < 0 && errno == EINTR);
            if (cbWrite < 0) win32::SetLastError(DwFileError());
        }
    }
    lockFiles.unlock();
    return (UINT)cbWrite;
}

DWORD win32::GetFileSize(HANDLE h, LPDWORD lpdwHigh) {
    COMPATFILE *lp;
    int64_t cb;
    DWORD dw = win32::INVALID_FILE_SIZE;
    INT_PTR id = (INT_PTR)h;

    std::unique_lock lockFiles(fFileLock);
    lp = id >= 0 && id <= INT_MAX ? LpFile((HFILE)id) : NULL;
    if (lp && (cb = CbNativeFile(lp->pf)) >= 0) {
        dw = (DWORD)(uint64_t)cb;
        if (lpdwHigh) *lpdwHigh = (DWORD)((uint64_t)cb >> 32);
        win32::SetLastError(win32::NO_ERROR);
    } else if (lp) win32::SetLastError(DwFileError());
    else win32::SetLastError(win32::ERROR_INVALID_HANDLE);
    lockFiles.unlock();
    return dw;
}

void CompatShutdownFiles(void) {
    COMPATFILE *lp;
    std::unique_lock lockFiles(fFileLock);
    while (lpFiles) {
        lp = lpFiles; lpFiles = lp->lpNext;
        ICloseNativeFile(lp->pf); free(lp);
    }
    lockFiles.unlock();
}
