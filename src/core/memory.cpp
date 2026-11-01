#include "common.h"
#include "appstate.h"
#include "text_io.h"
#include "heap_io.h"
#include "alert_io.h"
#include "liststorage.h"
#include <algorithm>


NATIVEGAMEHEAPS::NATIVEGAMEHEAPS(APPSESSION &app) noexcept : app(app), rglphb(app.state.rglphb) {}

NATIVEGAMEHEAPS::~NATIVEGAMEHEAPS() noexcept { Close(); }

void NATIVEGAMEHEAPS::Close() noexcept {
    for (HB *&lphb : rglphb) {
        HB *lphbFree = lphb;
        lphb = nullptr;
        FreeHb(app, lphbFree);
    }
}

std::uint16_t CbHeapAlign(HeapType ht) {
    /* NATIVE: ordinary heaps also hold native pointer arrays. Battle records
       are walked and written using their original two-byte boundaries. */
    return ht == htBattle ? 2 : alignof(std::max_align_t);
}

std::uint16_t CbHbHeader(HeapType ht) {
    std::uint16_t cbAlign = CbHeapAlign(ht);

    return ht == htBattle ? sizeof(HB) :
        ((sizeof(HB) + 2 + cbAlign - 1) / cbAlign) * cbAlign - 2;
}

std::uint16_t CbHeapItem(APPSESSION &app, std::uint16_t cb, HeapType ht) {
    std::uint32_t cbAlign = CbHeapAlign(ht);
    std::uint32_t cbItem = (static_cast<std::uint32_t>(cb) + 2 + cbAlign - 1) / cbAlign * cbAlign;

    if (cbItem > static_cast<std::uint32_t>(UINT16_MAX - CbHbHeader(ht))) {
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsMemory, nullptr), win32::MB_ICONHAND);
        StarsThrowError(starsErrorMemory, idsMemory);
    }
    return cbItem;
}

HB *LphbAlloc(APPSESSION &app, std::uint16_t cb, HeapType ht) {
    void *hmem;
    HB   *lphb;

    lphb = nullptr;
    cb += CbHbHeader(ht);
    if (cb < app.state.mphtcbAlloc[ht]) {
        cb = app.state.mphtcbAlloc[ht];
    }
    /* NATIVE: the original locked a zero-filled GlobalAlloc block. */
    hmem = std::calloc(1, cb);
    if (!hmem) {
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsMemory, nullptr), win32::MB_ICONHAND);
        StarsThrowError(starsErrorMemory, idsMemory);
    }
    // The lifetime operation retains calloc's initialized representation.
    hmem = std::memmove(hmem, hmem, cb);
    lphb = static_cast<HB *>(std::memmove(hmem, hmem, sizeof(HB)));
    lphb->hmem = hmem;
    lphb->cbBlock = cb;
    lphb->cbSlop = cb - CbHbHeader(ht);
    lphb->cbFree = cb - CbHbHeader(ht);
    lphb->ibTop = CbHbHeader(ht);
    lphb->ht = ht;
    lphb->lphbNext = app.state.rglphb[ht];
    app.state.rglphb[ht] = lphb;
    return lphb;
}

HB *LphbReAlloc(APPSESSION &app, HB *lphb) {
    void    *hmem;
    HB      *lphbT;
    HB      *lphbNew;
    HB     **plphb;
    std::uint16_t cbCur;
    std::uint16_t cbGrow;

    if (!lphb) {
        return nullptr;
    }
    hmem = lphb->hmem;
    cbCur = lphb->cbBlock;
    cbGrow = app.state.mphtcbAlloc[lphb->ht];
    if (cbCur >= 0xffdc)
        goto LReAllocOOM;
    if (cbCur > static_cast<std::uint16_t>((0xffdc - cbGrow))) {
        cbGrow = 0xffdc - cbCur;
    }
    // Keep the stable link slot before realloc invalidates the old address.
    if (app.state.rglphb[lphb->ht] == lphb) {
        plphb = &app.state.rglphb[lphb->ht];
    } else {
        for (lphbT = app.state.rglphb[lphb->ht]; lphbT && lphbT->lphbNext != lphb; lphbT = lphbT->lphbNext) {
        }
        plphb = &lphbT->lphbNext;
    }
    /* NATIVE: GlobalReAlloc zero-filled the bytes it added. */
    hmem = std::realloc(hmem, static_cast<std::size_t>((lphb->cbBlock + cbGrow)));
    if (!hmem) {
    LReAllocOOM:
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsMemory, nullptr), win32::MB_ICONHAND);
        StarsThrowError(starsErrorMemory, idsMemory);
    }
    hmem = std::memmove(hmem, hmem, static_cast<std::size_t>(cbCur) + cbGrow);
    std::fill_n(static_cast<std::byte *>(hmem) + cbCur, cbGrow, std::byte{});
    lphbNew = static_cast<HB *>(std::memmove(hmem, hmem, sizeof(HB)));
    lphbNew->hmem = hmem;
    *plphb = lphbNew;
    lphbNew->cbBlock += cbGrow;
    lphbNew->cbFree += cbGrow;
    lphbNew->cbSlop += cbGrow;
    return lphbNew;
}

void FreeHb(APPSESSION &app, HB *lphb) noexcept {
    void *hmem;
    HB   *lphbNext;

    // Detach before release so explicit shutdown and owner cleanup are both
    // idempotent. A subchain may be released without invalidating its prefix.
    if (lphb) {
        HB **plphb = &app.state.rglphb[lphb->ht];
        while (*plphb && *plphb != lphb) plphb = &(*plphb)->lphbNext;
        if (*plphb) *plphb = nullptr;
    }
    if (lphb) {
        for (; lphb; lphb = lphbNext) {
            lphbNext = lphb->lphbNext;
            hmem = lphb->hmem;
            std::free(hmem);
        }
    }
    return;
}

void ResetHb(APPSESSION &app, HeapType ht) {
    HB *lphb;

    for (lphb = app.state.rglphb[ht]; lphb; lphb = lphb->lphbNext) {
        lphb->ibTop = CbHbHeader(ht);
        lphb->cbSlop = lphb->cbBlock - CbHbHeader(ht);
        lphb->cbFree = lphb->cbBlock - CbHbHeader(ht);
    }
    return;
}

void *LpAlloc(APPSESSION &app, std::uint16_t cb, HeapType ht) {
    std::int16_t  fFree;
    std::uint16_t cbItem;
    std::byte *lpbPrev;
    std::byte *lpbTop;
    HB      *lphb;
    std::byte *lpb;

    lphb = app.state.rglphb[ht];
    cb = CbHeapItem(app, cb, ht);
    while (lphb && lphb->cbFree < cb) {
    LTryNextBlock:
        lphb = lphb->lphbNext;
    }
    if (!lphb) {
        lphb = LphbAlloc(app, cb, ht);
    }
    lpbTop = static_cast<std::byte *>(lphb->hmem) + lphb->ibTop;
    if (lphb->cbSlop >= cb) {
        RawStore16(lpbTop, cb - 2);
        lphb->ibTop += cb;
        lphb->cbFree -= cb;
        lphb->cbSlop -= cb;
        return lpbTop + 2;
    }
    lpb = static_cast<std::byte *>(lphb->hmem) + CbHbHeader(ht);
    while (lpb < lpbTop) {
        lpbPrev = lpb;
        fFree = RawLoad16(lpb) & 1;
        cbItem = RawLoad16(lpb) & 0xfffe;
        lpb += 2 + cbItem;
        if (fFree) {
            while (lpb < lpbTop && (RawLoad16(lpb) & 1) && lpb - lpbPrev < cb) {
                lpb += 2 + (RawLoad16(lpb) & 0xfffe);
            }
            cbItem = lpb - lpbPrev - 2;
            RawStore16(lpbPrev, cbItem | 1);
            if (static_cast<std::uint16_t>((cbItem + 2)) >= cb) {
                RawStore16(lpbPrev, RawLoad16(lpbPrev) & 0xfffe);
                lpbPrev += 2;
                lphb->cbFree -= cbItem + 2;
                return lpbPrev;
            }
        }
    }
    goto LTryNextBlock;
}

HB *LphbFromLpHt(APPSESSION &app, void *lp, HeapType ht) {
    HB *lphb;

    if (static_cast<std::int16_t>(ht) < htOrd || static_cast<std::int16_t>(ht) >= htCount) {
        return nullptr;
    }
    for (lphb = app.state.rglphb[ht]; lphb && (reinterpret_cast<std::uintptr_t>(lp) <= reinterpret_cast<std::uintptr_t>(lphb->hmem) || reinterpret_cast<std::uintptr_t>(lp) - reinterpret_cast<std::uintptr_t>(lphb->hmem) >= lphb->cbBlock); lphb = lphb->lphbNext) {
    }
    if (!lphb) {
        return nullptr;
    }
    return lphb;
}

void *LpReAlloc(APPSESSION &app, void *lp, std::uint16_t cb, HeapType ht) {
    void    *lpNew;
    HB      *lphb;
    std::uint16_t cbCur;
    std::uint16_t cbGrow;
    std::byte *lpb;

    lphb = LphbFromLpHt(app, lp, ht);
    lpb = static_cast<std::byte *>(lphb->hmem) +
          (reinterpret_cast<std::uintptr_t>(lp) - reinterpret_cast<std::uintptr_t>(lphb->hmem));
    cbCur = RawLoad16(lpb - 2);
    cb = CbHeapItem(app, cb, ht) - 2;
    cbGrow = cb - cbCur;
    if (cb <= cbCur) {
        return lp;
    }
LGrewHeap:
    if (static_cast<std::byte *>(lphb->hmem) + lphb->ibTop == lpb + cbCur && lphb->cbSlop >= cbGrow) {
        lphb->cbSlop -= cbGrow;
        lphb->cbFree -= cbGrow;
        lphb->ibTop += cbGrow;
        RawStore16(lpb - 2, cb);
    } else if (ht == htPlanets || ht == htThings) {
        lphb = LphbReAlloc(app, lphb);
        lpb = static_cast<std::byte *>(lphb->hmem) + (CbHbHeader(ht) + 2);
        lp = lpb;
        goto LGrewHeap;
    } else {
        lpNew = HeapIO().pfnAllocate(app, cb, ht);
        std::memcpy(lpNew, lpb, cbCur);
        HeapIO().pfnFree(app, lp, ht);
        lp = lpNew;
    }
    return lp;
}

void FreeLp(APPSESSION &app, void *lp, HeapType ht) {
    std::uint16_t cbFree;
    HB      *lphb;
    std::byte *lpb;

    if (lp) {
        lphb = LphbFromLpHt(app, lp, ht);
        lpb = static_cast<std::byte *>(lphb->hmem) +
              (reinterpret_cast<std::uintptr_t>(lp) - reinterpret_cast<std::uintptr_t>(lphb->hmem));
        cbFree = RawLoad16(lpb - 2) + 2;
        RawStore16(lpb - 2, RawLoad16(lpb - 2) | 1);
        lphb->cbFree += cbFree;
        if (lpb - static_cast<std::byte *>(lphb->hmem) + cbFree - 2 == lphb->ibTop) {
            lphb->ibTop -= cbFree;
            lphb->cbSlop += cbFree;
        }
    }
    return;
}

PL *LpplReAlloc(APPSESSION &app, PL *lppl, std::uint16_t cAlloc) {
    lppl = LpReAlloc<PL>(app, lppl, lppl->cbItem * cAlloc + 4, static_cast<HeapType>(lppl->ht));
    lppl->iMax = cAlloc;
    return lppl;
}

PL *LpplAlloc(APPSESSION &app, std::uint16_t cbItem, std::uint16_t cAlloc, HeapType ht) {
    PL *lppl;

    lppl = LpAlloc<PL>(app, cbItem * cAlloc + 4, ht);
    lppl->iMax = cAlloc;
    lppl->iMac = 0;
    lppl->fMark = 0;
    lppl->cbItem = cbItem;
    lppl->ht = ht;
    return lppl;
}

void FreePl(APPSESSION &app, PL *lppl) {
    if (lppl) {
        HeapIO().pfnFree(app, lppl, static_cast<HeapType>(lppl->ht));
    }
    return;
}

// The same pool allocation and four-byte prefix are retained, but callers now
// receive a PLPROD header and an actual PROD array instead of aliasing a PL.
template<>
PLPROD *LpplAlloc<PLPROD>(APPSESSION &app, std::uint16_t cbItem, std::uint16_t cAlloc, HeapType ht) {
    PL *lppl;

    lppl = LpplAlloc(app, cbItem, cAlloc, ht);
    return LpStartPl<PLPROD, PROD>(lppl, HeapIO().pfnExtent(app, lppl));
}

PLPROD *LpplReAlloc(APPSESSION &app, PLPROD *lppl, std::uint16_t cAlloc) {
    void *lp;

    lp = HeapIO().pfnResize(app, lppl, lppl->cbItem * cAlloc + 4, static_cast<HeapType>(lppl->ht));
    lppl = LpStartPl<PLPROD, PROD>(lp, HeapIO().pfnExtent(app, lp));
    lppl->iprodMax = cAlloc;
    return lppl;
}

void FreePl(APPSESSION &app, PLPROD *lppl) {
    if (lppl) HeapIO().pfnFree(app, lppl, static_cast<HeapType>(lppl->ht));
}

std::uint16_t CbPlSlot(APPSESSION &app, const void *lp) {
    for (int i = htOrd; i < htCount; i++) {
        HB *lphb = LphbFromLpHt(app, const_cast<void *>(lp), static_cast<HeapType>(i));
        if (lphb) {
            std::size_t ib = reinterpret_cast<std::uintptr_t>(lp) - reinterpret_cast<std::uintptr_t>(lphb->hmem);
            return RawLoad16(static_cast<const std::byte *>(lphb->hmem) + ib - 2);
        }
    }
    assert(false && "physical slot has no owning heap");
    return 0;
}

template<>
PLORD *LpplAlloc<PLORD>(APPSESSION &app, std::uint16_t cbItem, std::uint16_t cAlloc, HeapType ht) {
    PL *lppl;

    lppl = LpplAlloc(app, cbItem, cAlloc, ht);
    return LpStartPl<PLORD, ORDER>(lppl, HeapIO().pfnExtent(app, lppl));
}

PLORD *LpplReAlloc(APPSESSION &app, PLORD *lppl, std::uint16_t cAlloc) {
    void *lp;

    lp = HeapIO().pfnResize(app, lppl, lppl->cbItem * cAlloc + 4, static_cast<HeapType>(lppl->ht));
    lppl = LpStartPl<PLORD, ORDER>(lp, HeapIO().pfnExtent(app, lp));
    lppl->iordMax = cAlloc;
    return lppl;
}

void FreePl(APPSESSION &app, PLORD *lppl) {
    if (lppl) HeapIO().pfnFree(app, lppl, static_cast<HeapType>(lppl->ht));
}

// Legacy replay can address neighboring runtime bytes through a signed order
// index. Borrow the owning allocation, not an invented ORDER at that offset.
// Logical admission remains in the caller; out-of-allocation copies are outside
// the original defined domain. No view survives this callback-free operation.
static std::span<std::byte> RgbOrderStorage(APPSESSION &app, PLORD *lpplord, std::int16_t iord, std::int16_t ibExtra) {
    HB *lphb;
    std::int64_t ib;

    lphb = LphbFromLpHt(app, lpplord, htOrd);
    if (!lphb) return {};
    ib = static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(lpplord) - reinterpret_cast<std::uintptr_t>(lphb->hmem)) +
         4 + static_cast<std::int64_t>(iord) * static_cast<std::int64_t>(sizeof(ORDER)) + ibExtra;
    if (ib < 0 || static_cast<std::uint64_t>(ib) > lphb->cbBlock) return {};
    return std::span(static_cast<std::byte *>(lphb->hmem), lphb->cbBlock).subspan(static_cast<std::size_t>(ib));
}

bool FApplyOrderBytes(APPSESSION &app, PLORD *lpplord, std::int16_t iord, const std::uint8_t *rgb, std::int16_t cb, bool fZero) {
    std::span<std::byte> storage = RgbOrderStorage(app, lpplord, iord, 0);
    std::size_t cbCopy = static_cast<std::size_t>(cb - static_cast<std::int16_t>(cbWireWaypt));
    std::size_t ib = 0;
    std::size_t cbChunk, cbNative;
    ORDER ord{};

    static_assert(sizeof(ORDER) == cbWireOrder);
    if (fZero) {
        if (storage.size() < sizeof(ORDER)) return false;
        std::ranges::fill(storage.first(sizeof(ORDER)), std::byte{});
    }
    if (cbCopy > storage.size()) return false;
    // Wire records and the order heap are separate storage. A native temporary
    // permits byte-prefix effects without reading inactive/unaligned objects.
    do {
        cbNative = (std::min)(sizeof(ORDER), storage.size() - ib);
        if (cbNative == 0) return false;
        ord = {};
        std::memcpy(&ord, storage.data() + ib, cbNative);
        cbChunk = (std::min)(cbWireOrder, cbCopy - ib);
        FDecodeOrderPrefix(std::as_bytes(std::span(rgb + ib, cbChunk)), ord);
        if (ib == 0) ord.fNoAutoTrack = 0;
        std::memcpy(storage.data() + ib, &ord, cbNative);
        ib += cbChunk;
    } while (ib < cbCopy);
    return true;
}

bool FSetOrderTask(APPSESSION &app, PLORD *lpplord, std::int16_t iord, std::int16_t grTask) {
    std::span<std::byte> storage = RgbOrderStorage(app, lpplord, iord, cbWireOrderHeader - sizeof(std::uint16_t));
    ORDER ord{};
    auto rgbNative = std::as_writable_bytes(std::span(&ord, 1)).subspan(cbWireOrderHeader - sizeof(std::uint16_t), sizeof(std::uint16_t));

    static_assert(offsetof(ORDER, txp) == cbWireOrderHeader);
    if (storage.size() < sizeof(std::uint16_t)) return false;
    std::memcpy(rgbNative.data(), storage.data(), sizeof(std::uint16_t));
    ord.grTask = grTask;
    std::memcpy(storage.data(), rgbNative.data(), sizeof(std::uint16_t));
    return true;
}

// Byte views preserve physical spare/neighbor reads without pretending that a
// packed event or token object lives there. End positions remain valid bounds.
static NATIVEBYTEWINDOW BbwFromLpHt(APPSESSION &app, void *lp, HeapType ht) noexcept {
    HB *lphb;
    std::uintptr_t p, base;

    if (!lp) return {};
    p = reinterpret_cast<std::uintptr_t>(lp);
    for (lphb = app.state.rglphb[ht]; lphb; lphb = lphb->lphbNext) {
        base = reinterpret_cast<std::uintptr_t>(lphb->hmem);
        if (p >= base && p - base < lphb->cbBlock)
            return {std::span(static_cast<std::byte *>(lphb->hmem), lphb->cbBlock), static_cast<std::ptrdiff_t>(p - base)};
    }
    for (lphb = app.state.rglphb[ht]; lphb; lphb = lphb->lphbNext) {
        base = reinterpret_cast<std::uintptr_t>(lphb->hmem);
        if (p >= base && p - base == lphb->cbBlock)
            return {std::span(static_cast<std::byte *>(lphb->hmem), lphb->cbBlock), static_cast<std::ptrdiff_t>(p - base)};
    }
    assert(false && "byte view has no owning allocation");
    return {};
}

NATIVEBYTEWINDOW BbwFromLp(APPSESSION &app, void *lp) noexcept { return BbwFromLpHt(app, lp, htBattle); }
NATIVEBYTEWINDOW AbwFromLp(APPSESSION &app, void *lp) noexcept { return BbwFromLpHt(app, lp, htMisc); }
NATIVEBYTEWINDOW TbwFromLp(APPSESSION &app, void *lp) noexcept { return BbwFromLpHt(app, lp, htThings); }

void CopyThingBytes(APPSESSION &app, THING *pth, const void *rgb, std::uint16_t cb) noexcept {
    auto storage = TbwFromLp(app, pth).Rgb();
    assert(cb <= storage.size());
    std::memcpy(storage.data(), rgb, cb);
}

MSGPLR *LpmsgplrAlloc(APPSESSION &app, std::uint16_t cb) {
    void *lp = HeapIO().pfnAllocate(app, cb, htPlrMsg);
    return LpStartPl<MSGPLR, std::uint8_t>(lp, HeapIO().pfnExtent(app, lp));
}
MSGPLR *LpmsgplrReAlloc(APPSESSION &app, MSGPLR *lpmp, std::uint16_t cb) {
    void *lp = HeapIO().pfnResize(app, lpmp, cb, htPlrMsg);
    return LpStartPl<MSGPLR, std::uint8_t>(lp, HeapIO().pfnExtent(app, lp));
}

NATIVEBYTEWINDOW MbwFromLp(APPSESSION &app, void *lp) noexcept { return BbwFromLpHt(app, lp, htMsg); }
