"""Real typed queue lifetime operations over each witness's declared heap double."""
from check_s02_source import ROOT, function


def declarations(allocate=True, resize=True, free=True):
    text = '#include "' + str(ROOT / 'src/core/liststorage.h') + '"\n'
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
        text += "template<>\n" + function('src/core/memory.c', 'PLPROD *LpplAlloc<PLPROD>(') + "\n"
    if resize:
        text += function('src/core/memory.c', 'PLPROD *LpplReAlloc(') + "\n"
    if free:
        text += function('src/core/memory.c', 'void FreePl(PLPROD *') + "\n"
    return text
