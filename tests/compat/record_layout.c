// Compare the frozen independent C declarations with current C++ storage.
#include "../reference/cpp-a/common.h"
#define STARS_LAYOUT(type) printf(#type " %zu %zu\n", sizeof(type), (size_t)_Alignof(type))
int main(void) {
#include "record_layout.inc"
    return 0;
}
