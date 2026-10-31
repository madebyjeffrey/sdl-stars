"""Real typed queue lifetime operations over each witness's declared heap double."""
from check_s02_source import ROOT, function


def declarations(allocate=True, resize=True, free=True):
    text = '#include "' + str(ROOT / 'src/core/liststorage.h') + '"\n'
    if allocate or resize:
        header = (ROOT / 'src/core/memory.h').read_text()
        text += '#ifndef STARS_DECOMPILED_MEMORY_H\n#include <cassert>\n'
        text += 'uint16_t CbPlSlot(const void *);\nvoid *LpAlloc(uint16_t,HeapType);\nvoid *LpReAlloc(void *,uint16_t,HeapType);\n'
        text += header[header.index('// Retain every initialized physical slot byte'):header.rindex('#endif')]
        text += '#endif\n'
    if allocate:
        text += "PL *LpplAlloc(uint16_t,uint16_t,HeapType);\n"
        text += "template<class T> T *LpplAlloc(uint16_t,uint16_t,HeapType);\n"
        text += "template<> PLPROD *LpplAlloc<PLPROD>(uint16_t,uint16_t,HeapType);\n"
    if resize:
        text += "PLPROD *LpplReAlloc(PLPROD *,uint16_t);\n"
    if free:
        text += "void FreePl(PLPROD *);\n"
    return text


def definitions(extent="", allocate=True, resize=True, free=True):
    text = ""
    if allocate or resize:
        text += "uint16_t CbPlSlot(const void *lp) {" + extent + "}\n"
    if allocate:
        text += "template<>\n" + function('src/core/memory.cpp', 'PLPROD *LpplAlloc<PLPROD>(') + "\n"
    if resize:
        text += function('src/core/memory.cpp', 'PLPROD *LpplReAlloc(') + "\n"
    if free:
        text += function('src/core/memory.cpp', 'void FreePl(PLPROD *') + "\n"
    return text
