#ifndef STARS_NATIVE_PROCESS_H
#define STARS_NATIVE_PROCESS_H
#include <cstdint>

#ifdef _WIN32
#include <optional>
#include <string>
#include <vector>
[[nodiscard]] std::optional<std::vector<std::string>> RgNativeArguments();
#endif

// Milliseconds used for retries and file salts, with the platform tick behavior.
std::uint32_t DwTickCount();
#endif
