#ifndef STARS_DECOMPILED_MEMORY_H
#define STARS_DECOMPILED_MEMORY_H

#include "common.h"
#include <stdint.h>
#include <cassert>
#include <cstring>
#include <new>
#include <type_traits>

// Own the chains indexed by the legacy roots without moving any block. Reset
// retains the storage; Close releases it. The root array is the single index.
class NATIVEGAMEHEAPS {
    HB *(&rglphb)[12];
public:
    explicit NATIVEGAMEHEAPS(HB *(&roots)[12]) noexcept : rglphb(roots) {}
    ~NATIVEGAMEHEAPS() noexcept;
    NATIVEGAMEHEAPS(const NATIVEGAMEHEAPS &) = delete;
    NATIVEGAMEHEAPS &operator=(const NATIVEGAMEHEAPS &) = delete;
    void Close() noexcept;
};

HB   *LphbAlloc(uint16_t cb, HeapType ht);
HB   *LphbReAlloc(HB *lphb);
void  FreeHb(HB *lphb) noexcept;
void  ResetHb(HeapType ht);
void *LpAlloc(uint16_t cb, HeapType ht);
HB   *LphbFromLpHt(void *lp, HeapType ht);
void *LpReAlloc(void *lp, uint16_t cb, HeapType ht);
void  FreeLp(void *lp, HeapType ht);
PL   *LpplReAlloc(PL *lppl, uint16_t cAlloc);
PL   *LpplAlloc(uint16_t cbItem, uint16_t cAlloc, HeapType ht);
void  FreePl(PL *lppl);
uint16_t CbPlSlot(const void *lp);
template<class T> T *LpplAlloc(uint16_t cbItem, uint16_t cAlloc, HeapType ht);
template<> PLPROD *LpplAlloc<PLPROD>(uint16_t cbItem, uint16_t cAlloc, HeapType ht);
PLPROD *LpplReAlloc(PLPROD *lppl, uint16_t cAlloc);
void FreePl(PLPROD *lppl);
template<> PLORD *LpplAlloc<PLORD>(uint16_t cbItem, uint16_t cAlloc, HeapType ht);
PLORD *LpplReAlloc(PLORD *lppl, uint16_t cAlloc);
void FreePl(PLORD *lppl);
bool FApplyOrderBytes(PLORD *lpplord, int16_t iord, const uint8_t *rgb, int16_t cb, bool fZero);
bool FSetOrderTask(PLORD *lpplord, int16_t iord, int16_t grTask);

void CopyThingBytes(THING *pth, const void *rgb, uint16_t cb) noexcept;

MSGPLR *LpmsgplrAlloc(uint16_t cb);
MSGPLR *LpmsgplrReAlloc(MSGPLR *lpmp, uint16_t cb);

// Retain every initialized physical slot byte, including rounded spare items.
// memmove is a C++20 implicit-lifetime operation; no value initialization,
// allocation, logical count gate or moving container is introduced here.
template<class T> T *LpStartHeapArray(void *lp, std::size_t cb) noexcept {
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_default_constructible_v<T>);
    assert(reinterpret_cast<uintptr_t>(lp) % alignof(T) == 0);
    return std::launder(static_cast<T *>(std::memmove(lp, lp, cb)));
}
template<class T> T *LpAlloc(uint16_t cb, HeapType ht) {
    void *lp = LpAlloc(cb, ht);
    return LpStartHeapArray<T>(lp, CbPlSlot(lp));
}
template<class T> T *LpReAlloc(T *lp, uint16_t cb, HeapType ht) {
    void *lpNew = LpReAlloc(static_cast<void *>(lp), cb, ht);
    return LpStartHeapArray<T>(lpNew, CbPlSlot(lpNew));
}

#endif
