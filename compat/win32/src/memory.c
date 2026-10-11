/* Fixed pointers and movable heap handles stay distinct from immutable resources. */
#include "internal.h"
#include <stdatomic.h>
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
static atomic_flag fMemoryLock = ATOMIC_FLAG_INIT;
static UINT_PTR idMemory = 1;
static _Thread_local DWORD dwError;

DWORD WINAPI GetLastError(void) { return dwError; }
void WINAPI SetLastError(DWORD dw) { dwError = dw; }

static void LockMemory(void) {
    while (atomic_flag_test_and_set_explicit(&fMemoryLock, memory_order_acquire)) {}
}

static void UnlockMemory(void) {
    atomic_flag_clear_explicit(&fMemoryLock, memory_order_release);
}

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
    lp = calloc(1, sizeof(*lp));
    if (!lp) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    lp->fMoveable = (flags & GMEM_MOVEABLE) != 0;
    lp->cb = cb;
    if (cb != 0 || !lp->fMoveable) {
        lp->rgb = flags & GMEM_ZEROINIT ? calloc(1, cb ? cb : 1) : malloc(cb ? cb : 1);
        if (!lp->rgb) {
            free(lp); SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
    }
    LockMemory();
    // Movable handles are tagged IDs, never pointers into an allocation header.
    lp->h = lp->fMoveable ? (HANDLE)(idMemory++ * 16 + 3) : (HANDLE)lp->rgb;
    lp->lpNext = lpMemory;
    lpMemory = lp;
    UnlockMemory();
    return lp->h;
}

HGLOBAL WINAPI GlobalFree(HGLOBAL h) {
    COMPATMEMORY **lplp, *lp;

    if (!h)
        return NULL;
    LockMemory();
    for (lplp = &lpMemory; *lplp; lplp = &(*lplp)->lpNext) {
        lp = *lplp;
        if (lp->h == h) {
            *lplp = lp->lpNext;
            free(lp->rgb); free(lp);
            UnlockMemory();
            return NULL;
        }
    }
    UnlockMemory(); SetLastError(ERROR_INVALID_HANDLE);
    return h;
}

LPVOID WINAPI GlobalLock(HGLOBAL h) {
    COMPATMEMORY *lp;
    LPVOID lpv = NULL;

    LockMemory();
    lp = LpMemory(h);
    if (lp && lp->rgb) {
        if (lp->fMoveable && lp->cLocks < GMEM_LOCKCOUNT)
            lp->cLocks++;
        lpv = lp->rgb;
    }
    if (lp && !lp->rgb)
        SetLastError(ERROR_DISCARDED);
    UnlockMemory();
    return lpv;
}

BOOL WINAPI GlobalUnlock(HGLOBAL h) {
    COMPATMEMORY *lp;
    BOOL fRet = FALSE;

    LockMemory();
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
    UnlockMemory();
    return fRet;
}

SIZE_T WINAPI GlobalSize(HGLOBAL h) {
    COMPATMEMORY *lp;
    size_t cb = 0;

    LockMemory(); lp = LpMemory(h);
    if (lp) cb = lp->cb;
    UnlockMemory();
    return cb;
}

UINT WINAPI GlobalFlags(HGLOBAL h) {
    COMPATMEMORY *lp;
    UINT flags = GMEM_INVALID_HANDLE;

    LockMemory(); lp = LpMemory(h);
    if (lp) flags = lp->cLocks | (lp->rgb ? 0 : GMEM_DISCARDED);
    UnlockMemory();
    return flags;
}

HGLOBAL WINAPI GlobalHandle(LPCVOID lpv) {
    COMPATMEMORY *lp;
    HANDLE h = NULL;

    LockMemory();
    for (lp = lpMemory; lp; lp = lp->lpNext)
        if ((lpv && lp->rgb == lpv) || lp->h == lpv) { h = lp->h; break; }
    UnlockMemory();
    if (!h) SetLastError(ERROR_INVALID_HANDLE);
    return h;
}

HGLOBAL WINAPI GlobalReAlloc(HGLOBAL h, SIZE_T cb, UINT flags) {
    COMPATMEMORY *lp;
    BYTE *rgb;
    HANDLE hRet = NULL;

    LockMemory(); lp = LpMemory(h);
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
    rgb = realloc(lp->rgb, cb ? cb : 1);
    if (!rgb) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); goto LReturn; }
    if ((flags & GMEM_ZEROINIT) && cb > lp->cb)
        memset(rgb + lp->cb, 0, cb - lp->cb);
    lp->rgb = rgb; lp->cb = cb;
    if (!lp->fMoveable) lp->h = rgb;
    hRet = lp->h;
LReturn:
    UnlockMemory();
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

    LockMemory();
    while (lpMemory) {
        lp = lpMemory; lpMemory = lp->lpNext;
        free(lp->rgb); free(lp);
    }
    UnlockMemory();
}
