#ifndef STARS_DECOMPILED_MEMORY_H
#define STARS_DECOMPILED_MEMORY_H

#include "common.h"
#include <cstdint>
#include <cassert>
#include <cstring>
#include <new>
#include <type_traits>

// Own the chains indexed by the legacy roots without moving any block. Reset
// retains the storage; Close releases it. The root array is the single index.
class NATIVEGAMEHEAPS {
    APPSESSION &app;
    HB *(&rglphb)[12];
public:
    explicit NATIVEGAMEHEAPS(APPSESSION &app) noexcept;
    ~NATIVEGAMEHEAPS() noexcept;
    NATIVEGAMEHEAPS(const NATIVEGAMEHEAPS &) = delete;
    NATIVEGAMEHEAPS &operator=(const NATIVEGAMEHEAPS &) = delete;
    void Close() noexcept;
};

HB   *LphbAlloc(APPSESSION &app, std::uint16_t cb, HeapType ht);
HB   *LphbReAlloc(APPSESSION &app, HB *lphb);
void  FreeHb(APPSESSION &app, HB *lphb) noexcept;
void  ResetHb(APPSESSION &app, HeapType ht);
void *LpAlloc(APPSESSION &app, std::uint16_t cb, HeapType ht);
HB   *LphbFromLpHt(APPSESSION &app, void *lp, HeapType ht);
void *LpReAlloc(APPSESSION &app, void *lp, std::uint16_t cb, HeapType ht);
void  FreeLp(APPSESSION &app, void *lp, HeapType ht);
PL   *LpplReAlloc(APPSESSION &app, PL *lppl, std::uint16_t cAlloc);
PL   *LpplAlloc(APPSESSION &app, std::uint16_t cbItem, std::uint16_t cAlloc, HeapType ht);
void  FreePl(APPSESSION &app, PL *lppl);
std::uint16_t CbPlSlot(APPSESSION &app, const void *lp);
std::uint16_t CbHeapAlign(HeapType ht);
std::uint16_t CbHbHeader(HeapType ht);
std::uint16_t CbHeapItem(APPSESSION &app, std::uint16_t cb, HeapType ht);
template<class T> T *LpplAlloc(APPSESSION &app, std::uint16_t cbItem, std::uint16_t cAlloc, HeapType ht);
template<> PLPROD *LpplAlloc<PLPROD>(APPSESSION &app, std::uint16_t cbItem, std::uint16_t cAlloc, HeapType ht);
PLPROD *LpplReAlloc(APPSESSION &app, PLPROD *lppl, std::uint16_t cAlloc);
void FreePl(APPSESSION &app, PLPROD *lppl);
template<> PLORD *LpplAlloc<PLORD>(APPSESSION &app, std::uint16_t cbItem, std::uint16_t cAlloc, HeapType ht);
PLORD *LpplReAlloc(APPSESSION &app, PLORD *lppl, std::uint16_t cAlloc);
void FreePl(APPSESSION &app, PLORD *lppl);
bool FApplyOrderBytes(APPSESSION &app, PLORD *lpplord, std::int16_t iord, const std::uint8_t *rgb, std::int16_t cb, bool fZero);
bool FSetOrderTask(APPSESSION &app, PLORD *lpplord, std::int16_t iord, std::int16_t grTask);

void CopyThingBytes(APPSESSION &app, THING *pth, const void *rgb, std::uint16_t cb) noexcept;

MSGPLR *LpmsgplrAlloc(APPSESSION &app, std::uint16_t cb);
MSGPLR *LpmsgplrReAlloc(APPSESSION &app, MSGPLR *lpmp, std::uint16_t cb);

// Queue storage can supply an allocator while retaining the real lifetime helpers.
struct HEAPIO {
    void *(*pfnCopy)(void *, const void *, std::size_t) = std::memcpy;
    void *(*pfnMove)(void *, const void *, std::size_t) = std::memmove;
    void *(*pfnAllocate)(APPSESSION &, std::uint16_t, HeapType) = static_cast<void *(*)(APPSESSION &, std::uint16_t, HeapType)>(&LpAlloc);
    void *(*pfnResize)(APPSESSION &, void *, std::uint16_t, HeapType) = static_cast<void *(*)(APPSESSION &, void *, std::uint16_t, HeapType)>(&LpReAlloc);
    decltype(&FreeLp) pfnFree = FreeLp;
    decltype(&CbPlSlot) pfnExtent = CbPlSlot;
};
const HEAPIO &HeapIO() noexcept;
class HEAPIOSCOPE {
    HEAPIO io;
    const HEAPIO *lpPrev;
public:
    explicit HEAPIOSCOPE(const HEAPIO &io) noexcept;
    ~HEAPIOSCOPE() noexcept;
    HEAPIOSCOPE(const HEAPIOSCOPE &) = delete;
    HEAPIOSCOPE &operator=(const HEAPIOSCOPE &) = delete;
};

// Retain every initialized physical slot byte, including rounded spare items.
// memmove is a C++20 implicit-lifetime operation; no value initialization,
// allocation, logical count gate or moving container is introduced here.
template<class T> T *LpStartHeapArray(void *lp, std::size_t cb) noexcept {
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_default_constructible_v<T>);
    assert(reinterpret_cast<std::uintptr_t>(lp) % alignof(T) == 0);
    return std::launder(static_cast<T *>(std::memmove(lp, lp, cb)));
}
template<class T> T *LpAlloc(APPSESSION &app, std::uint16_t cb, HeapType ht) {
    void *lp = HeapIO().pfnAllocate(app, cb, ht);
    return LpStartHeapArray<T>(lp, HeapIO().pfnExtent(app, lp));
}
template<class T> T *LpReAlloc(APPSESSION &app, T *lp, std::uint16_t cb, HeapType ht) {
    void *lpNew = HeapIO().pfnResize(app, static_cast<void *>(lp), cb, ht);
    return LpStartHeapArray<T>(lpNew, HeapIO().pfnExtent(app, lpNew));
}

#endif
