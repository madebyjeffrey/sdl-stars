#include "locking.h"
/* Fixed pointers and movable heap handles stay distinct from immutable resources. */
#include "internal.h"
#include <stdlib.h>
#include <string.h>

typedef struct COMPATMEMORY {
    HANDLE h;
    BYTE *rgb;
    size_t cb;
    UINT cLocks;
    BOOL fMoveable;
    struct COMPATMEMORY *lpNext;
} COMPATMEMORY;

static COMPATMEMORY *lpMemory;
static NATIVEFLAGLOCK fMemoryLock;
static UINT_PTR idMemory = 1;
static thread_local DWORD dwError;

DWORD WINAPI GetLastError(void) { return dwError; }
void WINAPI SetLastError(DWORD dw) { dwError = dw; }

static COMPATMEMORY *LpMemory(HANDLE h) {
    COMPATMEMORY *lp;
    for (lp = lpMemory; lp; lp = lp->lpNext)
        if (lp->h == h)
            return lp;
    SetLastError(ERROR_INVALID_HANDLE);
    return NULL;
}

HGLOBAL WINAPI GlobalAlloc(UINT flags, SIZE_T cb) {
    COMPATMEMORY *lp;

    if (flags & ~(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DISCARDABLE | GMEM_NOCOMPACT | GMEM_NODISCARD | GMEM_SHARE)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    lp = (COMPATMEMORY *)(calloc(1, sizeof(*lp)));
    if (!lp) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    lp->fMoveable = (flags & GMEM_MOVEABLE) != 0;
    lp->cb = cb;
    if (cb != 0 || !lp->fMoveable) {
        lp->rgb = (BYTE *)(flags & GMEM_ZEROINIT ? calloc(1, cb ? cb : 1) : malloc(cb ? cb : 1));
        if (!lp->rgb) {
            free(lp); SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
    }
    std::unique_lock lockMemory(fMemoryLock);
    // Movable handles are tagged IDs, never pointers into an allocation header.
    lp->h = lp->fMoveable ? (HANDLE)(idMemory++ * 16 + 3) : (HANDLE)lp->rgb;
    lp->lpNext = lpMemory;
    lpMemory = lp;
    lockMemory.unlock();
    return lp->h;
}

HGLOBAL WINAPI GlobalFree(HGLOBAL h) {
    COMPATMEMORY **lplp, *lp;

    if (!h)
        return NULL;
    std::unique_lock lockMemory(fMemoryLock);
    for (lplp = &lpMemory; *lplp; lplp = &(*lplp)->lpNext) {
        lp = *lplp;
        if (lp->h == h) {
            *lplp = lp->lpNext;
            free(lp->rgb); free(lp);
            lockMemory.unlock();
            return NULL;
        }
    }
    lockMemory.unlock(); SetLastError(ERROR_INVALID_HANDLE);
    return h;
}

LPVOID WINAPI GlobalLock(HGLOBAL h) {
    COMPATMEMORY *lp;
    LPVOID lpv = NULL;

    std::unique_lock lockMemory(fMemoryLock);
    lp = LpMemory(h);
    if (lp && lp->rgb) {
        if (lp->fMoveable && lp->cLocks < GMEM_LOCKCOUNT)
            lp->cLocks++;
        lpv = lp->rgb;
    }
    if (lp && !lp->rgb)
        SetLastError(ERROR_DISCARDED);
    lockMemory.unlock();
    return lpv;
}

BOOL WINAPI GlobalUnlock(HGLOBAL h) {
    COMPATMEMORY *lp;
    BOOL fRet = FALSE;

    std::unique_lock lockMemory(fMemoryLock);
    lp = LpMemory(h);
    if (lp) {
        if (!lp->fMoveable) {
            fRet = TRUE;
        } else if (lp->cLocks != 0) {
            lp->cLocks--;
            fRet = lp->cLocks != 0;
            if (!fRet)
                SetLastError(NO_ERROR);
        } else {
            SetLastError(ERROR_NOT_LOCKED);
        }
    }
    lockMemory.unlock();
    return fRet;
}

SIZE_T WINAPI GlobalSize(HGLOBAL h) {
    COMPATMEMORY *lp;
    size_t cb = 0;

    std::unique_lock lockMemory(fMemoryLock); lp = LpMemory(h);
    if (lp) cb = lp->cb;
    lockMemory.unlock();
    return cb;
}

UINT WINAPI GlobalFlags(HGLOBAL h) {
    COMPATMEMORY *lp;
    UINT flags = GMEM_INVALID_HANDLE;

    std::unique_lock lockMemory(fMemoryLock); lp = LpMemory(h);
    if (lp) flags = lp->cLocks | (lp->rgb ? 0 : GMEM_DISCARDED);
    lockMemory.unlock();
    return flags;
}

HGLOBAL WINAPI GlobalHandle(LPCVOID lpv) {
    COMPATMEMORY *lp;
    HANDLE h = NULL;

    std::unique_lock lockMemory(fMemoryLock);
    for (lp = lpMemory; lp; lp = lp->lpNext)
        if ((lpv && lp->rgb == lpv) || lp->h == lpv) { h = lp->h; break; }
    lockMemory.unlock();
    if (!h) SetLastError(ERROR_INVALID_HANDLE);
    return h;
}

HGLOBAL WINAPI GlobalReAlloc(HGLOBAL h, SIZE_T cb, UINT flags) {
    COMPATMEMORY *lp;
    BYTE *rgb;
    HANDLE hRet = NULL;

    std::unique_lock lockMemory(fMemoryLock); lp = LpMemory(h);
    if (!lp) goto LReturn;
    if (flags & ~(GMEM_MODIFY | GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DISCARDABLE | GMEM_NOCOMPACT | GMEM_NODISCARD | GMEM_SHARE)) {
        SetLastError(ERROR_INVALID_PARAMETER); goto LReturn;
    }
    if (flags & GMEM_MODIFY) {
        if (!lp->fMoveable && (flags & GMEM_MOVEABLE)) {
            lp->fMoveable = TRUE;
            lp->h = (HANDLE)(idMemory++ * 16 + 3);
        }
        hRet = lp->h; goto LReturn;
    }
    if (cb == 0 && lp->fMoveable && (flags & GMEM_MOVEABLE)) {
        if (lp->cLocks != 0) { SetLastError(ERROR_LOCKED); goto LReturn; }
        free(lp->rgb); lp->rgb = NULL; lp->cb = 0;
        hRet = lp->h; goto LReturn;
    }
    // Locked or fixed blocks may resize in place without invalidating live pointers.
    if ((!lp->fMoveable || lp->cLocks != 0) && !(flags & GMEM_MOVEABLE)) {
        if (cb <= lp->cb) { lp->cb = cb; hRet = lp->h; }
        else SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto LReturn;
    }
    rgb = (BYTE *)(realloc(lp->rgb, cb ? cb : 1));
    if (!rgb) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); goto LReturn; }
    if ((flags & GMEM_ZEROINIT) && cb > lp->cb)
        memset(rgb + lp->cb, 0, cb - lp->cb);
    lp->rgb = rgb; lp->cb = cb;
    if (!lp->fMoveable) lp->h = rgb;
    hRet = lp->h;
LReturn:
    lockMemory.unlock();
    return hRet;
}

HLOCAL WINAPI LocalAlloc(UINT flags, SIZE_T cb) { return GlobalAlloc(flags, cb); }
HLOCAL WINAPI LocalFree(HLOCAL h) { return GlobalFree(h); }
LPVOID WINAPI LocalLock(HLOCAL h) { return GlobalLock(h); }
BOOL WINAPI LocalUnlock(HLOCAL h) { return GlobalUnlock(h); }
SIZE_T WINAPI LocalSize(HLOCAL h) { return GlobalSize(h); }
UINT WINAPI LocalFlags(HLOCAL h) { return GlobalFlags(h); }
HLOCAL WINAPI LocalReAlloc(HLOCAL h, SIZE_T cb, UINT flags) { return GlobalReAlloc(h, cb, flags); }
HLOCAL WINAPI LocalHandle(LPCVOID lpv) { return GlobalHandle(lpv); }

void CompatShutdownMemory(void) {
    COMPATMEMORY *lp;

    std::unique_lock lockMemory(fMemoryLock);
    while (lpMemory) {
        lp = lpMemory; lpMemory = lp->lpNext;
        free(lp->rgb); free(lp);
    }
    lockMemory.unlock();
}

#ifdef STARS_TEST_MEMORY_OBSERVE
size_t CCompatMemoryBlocks() {
    size_t c = 0;
    std::unique_lock lockMemory(fMemoryLock);
    for (COMPATMEMORY *lp = lpMemory; lp; lp = lp->lpNext) c++;
    return c;
}
#endif
