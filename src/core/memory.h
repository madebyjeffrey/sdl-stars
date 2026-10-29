#ifndef STARS_DECOMPILED_MEMORY_H
#define STARS_DECOMPILED_MEMORY_H

#include "common.h"
#include <stdint.h>

HB   *LphbAlloc(uint16_t cb, HeapType ht);
HB   *LphbReAlloc(HB *lphb);
void  FreeHb(HB *lphb);
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

#endif
