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
static void FreeMemoryBlock(COMPATMEMORY *lp) noexcept {
    free(lp->rgb);
    free(lp);
}

// One chain owns every registered block; callers retain borrowed handle tokens.
class COMPATMEMORYOWNER {
    COMPATMEMORY *&first;
    NATIVEFLAGLOCK &lock;
public:
    COMPATMEMORYOWNER(COMPATMEMORY *&first, NATIVEFLAGLOCK &lock) noexcept : first(first), lock(lock) {}
    ~COMPATMEMORYOWNER() noexcept { Close(); }
    COMPATMEMORYOWNER(const COMPATMEMORYOWNER &) = delete;
    COMPATMEMORYOWNER &operator=(const COMPATMEMORYOWNER &) = delete;
    void Close() noexcept {
        std::unique_lock guard(lock);
        while (first) {
            auto *lp = first;
            first = lp->lpNext;
            NATIVEOWNER<COMPATMEMORY, FreeMemoryBlock> block(lp);
        }
    }
};
static COMPATMEMORYOWNER memory(lpMemory, fMemoryLock);

static COMPATMEMORY *LpMemory(HANDLE h) {
    COMPATMEMORY *lp;
    for (lp = lpMemory; lp; lp = lp->lpNext)
        if (lp->h == h)
            return lp;
    win32::SetLastError(ERROR_INVALID_HANDLE);
    return NULL;
}

HGLOBAL WINAPI win32::GlobalAlloc(UINT flags, SIZE_T cb) {
    COMPATMEMORY *lp;
    NATIVEOWNER<COMPATMEMORY, FreeMemoryBlock> block;

    if (flags & ~(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DISCARDABLE | GMEM_NOCOMPACT | GMEM_NODISCARD | GMEM_SHARE)) {
        win32::SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    lp = (COMPATMEMORY *)(calloc(1, sizeof(*lp)));
    if (!lp) {
        win32::SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    block.reset(lp);
    lp->fMoveable = (flags & GMEM_MOVEABLE) != 0;
    lp->cb = cb;
    if (cb != 0 || !lp->fMoveable) {
        lp->rgb = (BYTE *)(flags & GMEM_ZEROINIT ? calloc(1, cb ? cb : 1) : malloc(cb ? cb : 1));
        if (!lp->rgb) {
            win32::SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
    }
    std::unique_lock lockMemory(fMemoryLock);
    // Movable handles are tagged IDs, never pointers into an allocation header.
    lp->h = lp->fMoveable ? (HANDLE)(idMemory++ * 16 + 3) : (HANDLE)lp->rgb;
    lp->lpNext = lpMemory;
    lpMemory = block.release();
    lockMemory.unlock();
    return lp->h;
}

HGLOBAL WINAPI win32::GlobalFree(HGLOBAL h) {
    COMPATMEMORY **lplp, *lp;

    if (!h)
        return NULL;
    std::unique_lock lockMemory(fMemoryLock);
    for (lplp = &lpMemory; *lplp; lplp = &(*lplp)->lpNext) {
        lp = *lplp;
        if (lp->h == h) {
            *lplp = lp->lpNext;
            FreeMemoryBlock(lp);
            lockMemory.unlock();
            return NULL;
        }
    }
    lockMemory.unlock(); win32::SetLastError(ERROR_INVALID_HANDLE);
    return h;
}

LPVOID WINAPI win32::GlobalLock(HGLOBAL h) {
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
        win32::SetLastError(ERROR_DISCARDED);
    lockMemory.unlock();
    return lpv;
}

BOOL WINAPI win32::GlobalUnlock(HGLOBAL h) {
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
                win32::SetLastError(NO_ERROR);
        } else {
            win32::SetLastError(ERROR_NOT_LOCKED);
        }
    }
    lockMemory.unlock();
    return fRet;
}

SIZE_T WINAPI win32::GlobalSize(HGLOBAL h) {
    COMPATMEMORY *lp;
    size_t cb = 0;

    std::unique_lock lockMemory(fMemoryLock); lp = LpMemory(h);
    if (lp) cb = lp->cb;
    lockMemory.unlock();
    return cb;
}

UINT WINAPI win32::GlobalFlags(HGLOBAL h) {
    COMPATMEMORY *lp;
    UINT flags = GMEM_INVALID_HANDLE;

    std::unique_lock lockMemory(fMemoryLock); lp = LpMemory(h);
    if (lp) flags = lp->cLocks | (lp->rgb ? 0 : GMEM_DISCARDED);
    lockMemory.unlock();
    return flags;
}

HGLOBAL WINAPI win32::GlobalHandle(LPCVOID lpv) {
    COMPATMEMORY *lp;
    HANDLE h = NULL;

    std::unique_lock lockMemory(fMemoryLock);
    for (lp = lpMemory; lp; lp = lp->lpNext)
        if ((lpv && lp->rgb == lpv) || lp->h == lpv) { h = lp->h; break; }
    lockMemory.unlock();
    if (!h) win32::SetLastError(ERROR_INVALID_HANDLE);
    return h;
}

HGLOBAL WINAPI win32::GlobalReAlloc(HGLOBAL h, SIZE_T cb, UINT flags) {
    COMPATMEMORY *lp;
    BYTE *rgb;
    HANDLE hRet = NULL;

    std::unique_lock lockMemory(fMemoryLock); lp = LpMemory(h);
    if (!lp) goto LReturn;
    if (flags & ~(GMEM_MODIFY | GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DISCARDABLE | GMEM_NOCOMPACT | GMEM_NODISCARD | GMEM_SHARE)) {
        win32::SetLastError(ERROR_INVALID_PARAMETER); goto LReturn;
    }
    if (flags & GMEM_MODIFY) {
        if (!lp->fMoveable && (flags & GMEM_MOVEABLE)) {
            lp->fMoveable = TRUE;
            lp->h = (HANDLE)(idMemory++ * 16 + 3);
        }
        hRet = lp->h; goto LReturn;
    }
    if (cb == 0 && lp->fMoveable && (flags & GMEM_MOVEABLE)) {
        if (lp->cLocks != 0) { win32::SetLastError(ERROR_LOCKED); goto LReturn; }
        free(lp->rgb); lp->rgb = NULL; lp->cb = 0;
        hRet = lp->h; goto LReturn;
    }
    // Locked or fixed blocks may resize in place without invalidating live pointers.
    if ((!lp->fMoveable || lp->cLocks != 0) && !(flags & GMEM_MOVEABLE)) {
        if (cb <= lp->cb) { lp->cb = cb; hRet = lp->h; }
        else win32::SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto LReturn;
    }
    rgb = (BYTE *)(realloc(lp->rgb, cb ? cb : 1));
    if (!rgb) { win32::SetLastError(ERROR_NOT_ENOUGH_MEMORY); goto LReturn; }
    if ((flags & GMEM_ZEROINIT) && cb > lp->cb)
        memset(rgb + lp->cb, 0, cb - lp->cb);
    lp->rgb = rgb; lp->cb = cb;
    if (!lp->fMoveable) lp->h = rgb;
    hRet = lp->h;
LReturn:
    lockMemory.unlock();
    return hRet;
}

HLOCAL WINAPI win32::LocalAlloc(UINT flags, SIZE_T cb) { return win32::GlobalAlloc(flags, cb); }
HLOCAL WINAPI win32::LocalFree(HLOCAL h) { return win32::GlobalFree(h); }
LPVOID WINAPI win32::LocalLock(HLOCAL h) { return win32::GlobalLock(h); }
BOOL WINAPI win32::LocalUnlock(HLOCAL h) { return win32::GlobalUnlock(h); }
SIZE_T WINAPI win32::LocalSize(HLOCAL h) { return win32::GlobalSize(h); }
UINT WINAPI win32::LocalFlags(HLOCAL h) { return win32::GlobalFlags(h); }
HLOCAL WINAPI win32::LocalReAlloc(HLOCAL h, SIZE_T cb, UINT flags) { return win32::GlobalReAlloc(h, cb, flags); }
HLOCAL WINAPI win32::LocalHandle(LPCVOID lpv) { return win32::GlobalHandle(lpv); }

void CompatShutdownMemory(void) { memory.Close(); }

#ifdef STARS_TEST_MEMORY_OBSERVE
size_t CCompatMemoryBlocks() {
    size_t c = 0;
    std::unique_lock lockMemory(fMemoryLock);
    for (COMPATMEMORY *lp = lpMemory; lp; lp = lp->lpNext) c++;
    return c;
}
#endif
