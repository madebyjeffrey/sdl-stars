// Check fixed underlying types and historical integer boundary bit patterns.
#include <type_traits>
#include <limits>
#include <cstring>
#include <bit>
#include "common.h"

#define STARS_ENUM_ABI(name, storage) \
    static_assert(std::is_enum<name>::value, #name " must be an enum"); \
    static_assert(std::is_same<std::underlying_type<name>::type, storage>::value, #name " storage changed");

#include "enum_storage.inc"
#undef STARS_ENUM_ABI

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "enum ABI line %d: %s\n", __LINE__, #f); return 1; } } while (0)
int main() {
    int cTypes = 0;
#define STARS_ENUM_ABI(name, storage) { \
    const storage rg[] = {0, 1, (std::numeric_limits<storage>::min)(), \
                          (std::numeric_limits<storage>::max)(), static_cast<storage>(-1), static_cast<storage>(0xa5a5)}; \
    for (storage raw : rg) { \
        name value = static_cast<name>(raw); \
        CHECK(std::bit_cast<storage>(value) == raw); \
        CHECK(std::memcmp(&value, &raw, sizeof(raw)) == 0); \
    } \
    ++cTypes; \
}
#include "enum_storage.inc"
#undef STARS_ENUM_ABI
    printf("Fixed enum storage: %d types, six boundary bit patterns each\n", cTypes);
    return 0;
}
