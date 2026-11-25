#include "common.h"
#define STARS_LAYOUT(type) printf(#type " %zu %zu\n", sizeof(type), alignof(type))
int main() {
#include "record_layout.inc"
    return 0;
}
