find_program(STARS_CARGO_EXECUTABLE cargo REQUIRED)
if(CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "The Bevy startup experiment currently requires a native toolchain")
endif()
set(STARS_BEVY_CRATE "${PROJECT_SOURCE_DIR}/src/ui/bevy_start")
set(STARS_BEVY_TARGET_DIR "${CMAKE_BINARY_DIR}/bevy-cargo")
if(WIN32)
    set(STARS_BEVY_RUST_TARGET "x86_64-pc-windows-gnu")
    set(STARS_BEVY_ARCHIVE "${STARS_BEVY_TARGET_DIR}/${STARS_BEVY_RUST_TARGET}/debug/libstars_bevy_start.a")
elseif(APPLE)
    set(STARS_BEVY_ARCHIVE "${STARS_BEVY_TARGET_DIR}/debug/libstars_bevy_start.a")
    if(CMAKE_OSX_ARCHITECTURES AND NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        message(FATAL_ERROR "The macOS Bevy experiment currently requires a native arm64 build")
    endif()
else()
    set(STARS_BEVY_ARCHIVE "${STARS_BEVY_TARGET_DIR}/debug/libstars_bevy_start.a")
endif()
set(STARS_BEVY_LINK_FLAGS "${STARS_BEVY_TARGET_DIR}/native-libs.rsp")
add_custom_command(OUTPUT "${STARS_BEVY_ARCHIVE}" "${STARS_BEVY_LINK_FLAGS}"
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/cmake/build_bevy.py"
        --cargo "${STARS_CARGO_EXECUTABLE}" --manifest "${STARS_BEVY_CRATE}/Cargo.toml"
        --target-dir "${STARS_BEVY_TARGET_DIR}" --link-flags "${STARS_BEVY_LINK_FLAGS}"
        --target "${STARS_BEVY_RUST_TARGET}"
    DEPENDS "${STARS_BEVY_CRATE}/Cargo.toml" "${STARS_BEVY_CRATE}/Cargo.lock"
        "${STARS_BEVY_CRATE}/src/lib.rs" "${PROJECT_SOURCE_DIR}/cmake/build_bevy.py"
        "${PROJECT_SOURCE_DIR}/res/fonts/LiberationSans-Regular.ttf"
    COMMENT "Building the Rust/Bevy startup screen" VERBATIM)
add_custom_target(stars-bevy-start-build DEPENDS "${STARS_BEVY_ARCHIVE}" "${STARS_BEVY_LINK_FLAGS}")
add_library(stars_bevy_start STATIC IMPORTED)
set_target_properties(stars_bevy_start PROPERTIES IMPORTED_LOCATION "${STARS_BEVY_ARCHIVE}")
add_dependencies(stars_bevy_start stars-bevy-start-build)
add_library(stars-bevy-ui OBJECT src/ui/bevystart.c)
target_link_libraries(stars-bevy-ui PRIVATE stars_options stars_win32_headers SDL3::SDL3)
target_compile_options(stars-bevy-ui PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(stars-sdl PRIVATE STARS_BEVY_STARTUP=1)
target_link_libraries(stars-sdl PRIVATE stars-bevy-ui stars_bevy_start "-Wl,@${STARS_BEVY_LINK_FLAGS}")
if(APPLE)
    # The unoptimized Rust engine exceeds Mach-O's compact unwind offset range.
    # Keep DWARF unwind information, which Rust's panic boundary still uses.
    target_link_options(stars-sdl PRIVATE "LINKER:-no_compact_unwind")
endif()
set_property(TARGET stars-sdl APPEND PROPERTY LINK_DEPENDS "${STARS_BEVY_LINK_FLAGS}")
