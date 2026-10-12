cmake_minimum_required(VERSION 3.23)
# The version this line is building toward. Tags (vX.Y.Z) and build numbers
# come from git; see cmake/version.cmake and docs/VERSIONING.md.
project(stars VERSION 2.9.0 LANGUAGES C)

# SDL is the only desktop backend. Host-only builds do not require SDL.
if(WIN32 AND NOT MINGW)
    message(FATAL_ERROR "On Windows, use the MSYS2 UCRT64 toolchain (docs/WINDOWS-SDL3.md)")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(STARS_BUILD_SDL_CLIENT "Build the SDL3 desktop client and wrapper" ON)
if(STARS_BUILD_SDL_CLIENT AND NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|Darwin|Windows)$")
    message(FATAL_ERROR "The SDL compatibility targets require Linux, macOS or Windows")
endif()

option(STARS_ENABLE_SANITIZERS "Instrument native Linux code with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
add_library(stars_sanitizers INTERFACE)
if(STARS_ENABLE_SANITIZERS)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "STARS_ENABLE_SANITIZERS requires a native Linux GCC or Clang compiler")
    endif()
    target_compile_options(stars_sanitizers INTERFACE
        -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
    target_link_options(stars_sanitizers INTERFACE -fsanitize=address,undefined)
endif()

# The game: rules, turn generation, AI and file I/O. A source file belongs
# here only if the command-line host needs it; dialogs and drawing go
# in STARS_UI_SOURCES and use the SDL wrapper.
set(STARS_CORE_SOURCES
    ai.c ai2.c ai3.c ai4.c aiutil.c battle.c build.c create.c file.c globals.c
    log.c memory.c mine.c msg.c native.c parts.c planet.c produce.c race.c
    report.c research.c save.c ship.c ship2.c strings.c
    thing.c turn.c turn2.c turn3.c util.c utilgen.c
)

# The shared desktop UI, implemented through the SDL Win32 wrapper.
set(STARS_UI_SOURCES
    battleui.c buildui.c createui.c fileui.c globalsui.c init.c logui.c mdi.c
    mineui.c msgui.c nativeui.c planetui.c planetpaneui.c popup.c produceui.c raceui.c
    reportui.c researchui.c scan.c scaninspect.c ship2ui.c shipui.c stars.c tb.c thingui.c
    tutor.c tutor2.c utilgenui.c utilui.c vcr.c
)

# Settings every target that compiles game code shares.
add_library(stars_options INTERFACE)
target_link_libraries(stars_options INTERFACE stars_sanitizers)

# Encoding boundaries are shared by the native host and SDL UI without game state.
add_library(stars_encoding STATIC encoding.c)
target_compile_options(stars_encoding INTERFACE "$<$<COMPILE_LANGUAGE:C>:SHELL:-iquote ${CMAKE_CURRENT_SOURCE_DIR}>")
target_link_libraries(stars_encoding PRIVATE stars_sanitizers)
target_compile_options(stars_encoding PRIVATE -Wall -Wextra -Werror)

# The core is shared by the SDL client, host and unit tests.
add_library(stars_core OBJECT ${STARS_CORE_SOURCES})
target_link_libraries(stars_core PUBLIC stars_options stars_encoding "$<$<PLATFORM_ID:Windows>:shell32>")

# The command-line host: the game code and host.c, no windows.
add_executable(stars-host host.c hostui.c)
target_link_libraries(stars-host PRIVATE stars_core)
set_target_properties(stars-host PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# version.h is regenerated on every build so the build number follows HEAD;
# it is rewritten only when the version changes.
set(STARS_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
# The platform label in the version shown (cmake/version.cmake): os-arch.
if(WIN32)
    set(STARS_PLATFORM_OS "windows")
elseif(APPLE)
    set(STARS_PLATFORM_OS "macos")
else()
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" STARS_PLATFORM_OS)
endif()
if(APPLE AND CMAKE_OSX_ARCHITECTURES)
    set(STARS_PLATFORM_ARCH "${CMAKE_OSX_ARCHITECTURES}")
else()
    set(STARS_PLATFORM_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
endif()
string(TOLOWER "${STARS_PLATFORM_ARCH}" STARS_PLATFORM_ARCH)
if(STARS_PLATFORM_ARCH MATCHES "^(x86_64|amd64)$")
    set(STARS_PLATFORM_ARCH "x64")
elseif(STARS_PLATFORM_ARCH MATCHES "^(arm64|aarch64)$")
    set(STARS_PLATFORM_ARCH "arm64")
elseif(STARS_PLATFORM_ARCH MATCHES ";")
    set(STARS_PLATFORM_ARCH "universal")
endif()
set(STARS_PLATFORM "${STARS_PLATFORM_OS}-${STARS_PLATFORM_ARCH}")

set(STARS_VERSION_COMMAND "${CMAKE_COMMAND}"
    -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    -DPLATFORM=${STARS_PLATFORM}
    -DOUTPUT=${STARS_GENERATED_DIR}/version.h
    -DVERSION_BASE=${PROJECT_VERSION}
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake")
file(MAKE_DIRECTORY "${STARS_GENERATED_DIR}")
execute_process(COMMAND ${STARS_VERSION_COMMAND})
add_custom_target(stars_version
    COMMAND ${STARS_VERSION_COMMAND}
    BYPRODUCTS "${STARS_GENERATED_DIR}/version.h"
    COMMENT "Checking Stars! version"
    VERBATIM
)
add_dependencies(stars_core stars_version)

# The game's text: text/textgen.py builds the ID enums (textids.h) and the
# compressed string tables from the rows in text/*.txt.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(STARS_TEXT_SOURCES text/strings.txt text/messages.txt text/tutorial.txt text/planets.txt)
list(TRANSFORM STARS_TEXT_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set(STARS_TEXT_OUTPUTS textids.h text_strings.c text_messages.c text_planets.c text_tutorial.c)
list(TRANSFORM STARS_TEXT_OUTPUTS PREPEND "${STARS_GENERATED_DIR}/")
add_custom_command(
    OUTPUT ${STARS_TEXT_OUTPUTS}
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/text/textgen.py" "${STARS_GENERATED_DIR}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/text/textgen.py" ${STARS_TEXT_SOURCES}
    COMMENT "Building the text tables"
    VERBATIM
)
add_custom_target(stars_text DEPENDS ${STARS_TEXT_OUTPUTS})

# The data tables: data/datagen.py builds dataids.h (the hull, part,
# starting design and AI recipe enums) and data_tables.c (the parts, races,
# default battle plans and AI recipes) from data/**/*.yaml. It needs PyYAML.
execute_process(COMMAND "${Python3_EXECUTABLE}" -c "import yaml" RESULT_VARIABLE STARS_PYYAML_MISSING OUTPUT_QUIET ERROR_QUIET)
if(STARS_PYYAML_MISSING)
    message(FATAL_ERROR "The build needs PyYAML for ${Python3_EXECUTABLE}. Install it (python3 -m pip install pyyaml), "
                        "or configure with -DPython3_EXECUTABLE=<a python that has it>.")
endif()
file(GLOB_RECURSE STARS_DATA_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/data/*.yaml")
set(STARS_DATA_OUTPUTS "${STARS_GENERATED_DIR}/dataids.h" "${STARS_GENERATED_DIR}/data_tables.c")
add_custom_command(
    OUTPUT ${STARS_DATA_OUTPUTS}
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/data/datagen.py" "${STARS_GENERATED_DIR}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/data/datagen.py" ${STARS_DATA_SOURCES}
    COMMENT "Building the data tables"
    VERBATIM
)
add_custom_target(stars_data DEPENDS ${STARS_DATA_OUTPUTS})

# Everything that includes enums.h needs textids.h and dataids.h first.
add_dependencies(stars_options stars_text stars_data)
target_sources(stars_core PRIVATE
    "${STARS_GENERATED_DIR}/text_strings.c"
    "${STARS_GENERATED_DIR}/text_messages.c"
    "${STARS_GENERATED_DIR}/text_planets.c"
    "${STARS_GENERATED_DIR}/data_tables.c"
)

# The game's headers are found for #include "..." only (-iquote): strings.h
# is the game's string table, and glibc's <string.h> includes the system
# <strings.h>. The resource compiler takes them as include directories.
foreach(STARS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/res" "${STARS_GENERATED_DIR}")
    target_compile_options(stars_options INTERFACE "$<$<COMPILE_LANGUAGE:C>:SHELL:-iquote ${STARS_INCLUDE_DIR}>")
    target_include_directories(stars_options INTERFACE "$<$<COMPILE_LANGUAGE:RC>:${STARS_INCLUDE_DIR}>")
endforeach()

# Match the existing MinGW compile-analysis flags. Plain char is signed, as
# in the original MSC build; the decompiler relies on it to drop char casts.
target_compile_options(stars_options INTERFACE
    $<$<COMPILE_LANGUAGE:C>:-Wno-pointer-sign>
    $<$<COMPILE_LANGUAGE:C>:-fsigned-char>
    $<$<COMPILE_LANGUAGE:C>:-fno-fast-math>
    $<$<COMPILE_LANGUAGE:C>:-ffp-contract=off>
)

# Binary64 operation boundaries are intentional; see docs/DOUBLE-PRECISION.md.
include(CheckCSourceCompiles)
check_c_source_compiles("
    #include <float.h>
    #if DBL_MANT_DIG != 53 || DBL_MAX_EXP != 1024 || FLT_EVAL_METHOD != 0
    #error unsupported double evaluation
    #endif
    int main(void) { return 0; }" STARS_BINARY64_DOUBLE)
if(NOT STARS_BINARY64_DOUBLE)
    message(FATAL_ERROR "Stars requires binary64 double without excess evaluation precision")
endif()

target_link_libraries(stars_options INTERFACE m)
option(STARS_BUILD_TESTS "Build game and wrapper tests" ON)
option(STARS_BUILD_WIN32_REFERENCE_TESTS "Build independent Windows SDK contract tests" OFF)
if(STARS_BUILD_TESTS)
    enable_testing()
endif()

if(STARS_BUILD_SDL_CLIENT)
    include(cmake/sdl-client.cmake)
elseif(STARS_BUILD_TESTS)
    set(STARS_TEST_DIR "${CMAKE_BINARY_DIR}/tests")
    file(MAKE_DIRECTORY "${STARS_TEST_DIR}/data")
    configure_file(tests/scaffold/fixtures/newgame/tiny/humanoid.r1 "${STARS_TEST_DIR}/data/humanoid.r1" COPYONLY)
    file(GLOB STARS_UNIT_TESTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/test_*.c")
    # These tests need the desktop UI and GNU linker file adapters.
    list(REMOVE_ITEM STARS_UNIT_TESTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/test_native_ports.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/test_native_fileui.c")
    set(STARS_HOST_TEST_TARGETS)
    foreach(STARS_UNIT_TEST IN LISTS STARS_UNIT_TESTS)
        get_filename_component(STARS_TEST_NAME "${STARS_UNIT_TEST}" NAME_WE)
        add_executable(${STARS_TEST_NAME} "${STARS_UNIT_TEST}" tests/unit/stars_test.c hostui.c)
        target_link_libraries(${STARS_TEST_NAME} PRIVATE stars_core)
        target_include_directories(${STARS_TEST_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit")
        target_compile_definitions(${STARS_TEST_NAME} PRIVATE STARS_TEST_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/golden")
        set_target_properties(${STARS_TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
        add_test(NAME ${STARS_TEST_NAME} COMMAND ${STARS_TEST_NAME})
        set_tests_properties(${STARS_TEST_NAME} PROPERTIES WORKING_DIRECTORY "${STARS_TEST_DIR}")
        list(APPEND STARS_HOST_TEST_TARGETS ${STARS_TEST_NAME})
    endforeach()
    add_custom_target(stars-host-unit-tests DEPENDS ${STARS_HOST_TEST_TARGETS})
endif()
if(NOT STARS_BUILD_SDL_CLIENT)
    include(cmake/install.cmake)
endif()
if(STARS_BUILD_WIN32_REFERENCE_TESTS)
    if(NOT WIN32)
        message(FATAL_ERROR "SDK reference tests require a Windows toolchain")
    endif()
    enable_language(RC)
    enable_testing()
    include(cmake/win32-reference-tests.cmake)
endif()
