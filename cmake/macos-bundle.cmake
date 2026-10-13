if(APPLE AND STARS_BUILD_TESTS)
    add_test(NAME macos_bundle_unit COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/packaging/test_macos_bundle.py")
    set_tests_properties(macos_bundle_unit PROPERTIES LABELS "packaging" TIMEOUT 30)
endif()
option(STARS_MACOS_BUNDLE "Enable the standalone macOS app packaging target" OFF)
if(NOT STARS_MACOS_BUNDLE)
    return()
endif()
if(NOT APPLE)
    message(FATAL_ERROR "STARS_MACOS_BUNDLE requires macOS")
endif()
if(NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    message(FATAL_ERROR "The macOS bundle currently targets Apple silicon (arm64)")
endif()
set(STARS_MACOS_SIGN_IDENTITY "AUTO" CACHE STRING "AUTO discovers a keychain identity; ADHOC is local-only; otherwise a signing identity hash/name")
add_custom_target(stars-macos-bundle
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/packaging/macos/bundle.py"
        --executable "$<TARGET_FILE:stars-sdl>"
        --help-directory "${STARS_HELP_GENERATED}"
        --version-header "${STARS_GENERATED_DIR}/version.h"
        --source "${PROJECT_SOURCE_DIR}"
        --output "${CMAKE_BINARY_DIR}/package/Stars!.app"
        --identity "${STARS_MACOS_SIGN_IDENTITY}"
        --cmake "${CMAKE_COMMAND}"
        --library-directory "$<TARGET_FILE_DIR:SDL3::SDL3>"
    DEPENDS stars-sdl stars-help-guide
    USES_TERMINAL VERBATIM
    COMMENT "Packaging and signing Stars!.app")
