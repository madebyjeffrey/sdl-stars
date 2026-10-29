// C11 callers retain the original integer typedefs at the public ABI.
#include "common.h"

#define STARS_ENUM_ABI(type, storage) \
    storage CRead##type(type value) { return value; } \
    type CReturn##type(storage value) { return value; } \
    void CStore##type(type *lp, storage value) { *lp = value; } \
    type CCall##type(type (*pfn)(type), type value) { return pfn(value); }
#include "enum_storage.inc"
#undef STARS_ENUM_ABI
