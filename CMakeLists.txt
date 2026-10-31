cmake_minimum_required(VERSION 3.23)
# The version this line is building toward. Tags (vX.Y.Z) and build numbers
# come from git; see cmake/version.cmake and docs/VERSIONING.md.
project(stars VERSION 2.9.0 LANGUAGES C CXX)

# SDL is the only desktop backend. Host-only builds do not require SDL.
if(WIN32 AND NOT MINGW)
    message(FATAL_ERROR "On Windows, use the MSYS2 UCRT64 toolchain (docs/WINDOWS-SDL3.md)")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OBJCXX_STANDARD 20)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(STARS_BUILD_SDL_CLIENT "Build the SDL3 desktop client and wrapper" ON)
if(STARS_BUILD_SDL_CLIENT AND NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|Darwin|Windows)$")
    message(FATAL_ERROR "The SDL compatibility targets require Linux, macOS or Windows")
endif()

option(STARS_ENABLE_SANITIZERS "Instrument native Linux/macOS code with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
add_library(stars_sanitizers INTERFACE)
if(STARS_ENABLE_SANITIZERS)
    if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|Darwin)$" OR NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "STARS_ENABLE_SANITIZERS requires a native Linux/macOS GCC or Clang compiler")
    endif()
    target_compile_options(stars_sanitizers INTERFACE
        -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
    target_link_options(stars_sanitizers INTERFACE -fsanitize=address,undefined)
endif()

# The game: rules, turn generation, AI and file I/O. A source file belongs
# here only if the command-line host needs it; dialogs and drawing go
# in STARS_UI_SOURCES and use the SDL wrapper.
set(STARS_CORE_SOURCES
    src/core/ai.cpp src/core/ai2.cpp src/core/ai3.cpp src/core/ai4.cpp src/core/aiutil.cpp src/core/battle.cpp src/core/build.cpp src/core/create.cpp src/core/file.cpp src/core/globals.cpp
    src/core/log.cpp src/core/memory.cpp src/core/mine.cpp src/core/msg.cpp src/platform/native.cpp src/core/parts.cpp src/core/planet.cpp src/core/produce.cpp src/core/race.cpp
    src/core/report.cpp src/core/research.cpp src/core/save.cpp src/core/ship.cpp src/core/ship2.cpp src/core/strings.cpp
    src/core/thing.cpp src/core/turn.cpp src/core/turn2.cpp src/core/turn3.cpp src/core/util.cpp src/core/utilgen.cpp
)

# The shared desktop UI, implemented through the SDL Win32 wrapper.
set(STARS_UI_SOURCES
    src/ui/battleui.cpp src/ui/buildui.cpp src/ui/createui.cpp src/ui/fileui.cpp src/ui/globalsui.cpp src/ui/init.cpp src/ui/logui.cpp src/ui/libraryui.cpp src/ui/libraryzip.cpp src/ui/mdi.cpp
    src/ui/mineui.cpp src/ui/msgui.cpp src/ui/nativeui.cpp src/ui/planetui.cpp src/ui/planetpaneui.cpp src/ui/popup.cpp src/ui/produceui.cpp src/ui/raceui.cpp
    src/ui/reportui.cpp src/ui/researchui.cpp src/ui/scan.cpp src/ui/scaninspect.cpp src/ui/ship2ui.cpp src/ui/shipui.cpp src/ui/stars.cpp src/ui/tb.cpp src/ui/thingui.cpp
    src/ui/tutor.cpp src/ui/tutor2.cpp src/ui/utilgenui.cpp src/ui/utilui.cpp src/ui/vcr.cpp
    src/ui/library_publication_io.cpp src/ui/library_file_io.cpp src/ui/control_io.cpp src/ui/desktop_choice_io.cpp src/ui/preferencesui.cpp src/ui/library_path_io.cpp
)

# These shared reference vectors compile as C through independent SDK entries.
# Their application entries use ordinary C++ source extensions.
# Their C11 zero-initializer spelling is intentional; isolate only its C++
# missing-field/braces diagnostics from ordinary C++ harnesses and production.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/cmake/win32-reference-tests.cmake")
file(READ "${PROJECT_SOURCE_DIR}/cmake/win32-reference-tests.cmake" STARS_CPP_SDK_REFERENCE_CMAKE)
string(REGEX MATCHALL "tests/compat/[A-Za-z0-9_]+\\.inc" STARS_CPP_SDK_REFERENCE_NAMES "${STARS_CPP_SDK_REFERENCE_CMAKE}")
list(REMOVE_DUPLICATES STARS_CPP_SDK_REFERENCE_NAMES)
set(STARS_CPP_SDK_SHARED_HARNESSES)
foreach(STARS_CPP_SDK_REFERENCE_NAME IN LISTS STARS_CPP_SDK_REFERENCE_NAMES)
    string(REGEX REPLACE "\\.inc$" ".cpp" STARS_CPP_SDK_REFERENCE_ENTRY "${STARS_CPP_SDK_REFERENCE_NAME}")
    list(APPEND STARS_CPP_SDK_SHARED_HARNESSES "${PROJECT_SOURCE_DIR}/${STARS_CPP_SDK_REFERENCE_ENTRY}")
endforeach()
set_source_files_properties(${STARS_CPP_SDK_SHARED_HARNESSES} PROPERTIES
    COMPILE_OPTIONS "-Wno-missing-field-initializers;-Wno-missing-braces")

# Error state and pending callback failures are shared by game and SDL runtime.
add_library(stars_errors STATIC src/platform/errors.cpp)
target_include_directories(stars_errors PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/platform")
target_link_libraries(stars_errors PRIVATE stars_sanitizers)
target_compile_options(stars_errors PRIVATE -Wall -Wextra -Werror)

# Settings every target that compiles game code shares.
add_library(stars_options INTERFACE)
target_link_libraries(stars_options INTERFACE stars_sanitizers stars_errors)

# Encoding boundaries are shared by the native host and SDL UI without game state.
add_library(stars_encoding STATIC src/platform/encoding.cpp)
target_compile_options(stars_encoding INTERFACE "$<$<COMPILE_LANGUAGE:C,CXX>:SHELL:-iquote ${CMAKE_CURRENT_SOURCE_DIR}/src/platform>")
target_link_libraries(stars_encoding PRIVATE stars_sanitizers)
target_compile_options(stars_encoding PRIVATE -Wall -Wextra -Werror)

add_library(stars_filesystem STATIC src/platform/filesystem.cpp src/platform/fileio.cpp src/platform/native_publish.cpp)
if(WIN32)
    target_sources(stars_filesystem PRIVATE src/platform/native_windows.cpp)
    target_link_libraries(stars_filesystem PRIVATE shell32)
endif()
set_target_properties(stars_filesystem PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_include_directories(stars_filesystem PUBLIC "${PROJECT_SOURCE_DIR}/src/platform")
target_link_libraries(stars_filesystem PRIVATE stars_encoding stars_sanitizers)
target_compile_options(stars_filesystem PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_link_libraries(stars_filesystem PRIVATE "$<$<PLATFORM_ID:Windows>:bcrypt>")

# The core is shared by the SDL client, host and unit tests.
add_library(stars_core OBJECT ${STARS_CORE_SOURCES}
    src/core/record_io.cpp
    src/core/random_io.cpp
    src/core/capacity_io.cpp
    src/core/alert_io.cpp
    src/core/player_choice_io.cpp
    src/core/text_io.cpp
    src/core/host_profile.cpp
    src/core/race_query_io.cpp
    src/core/part_query_io.cpp
    src/core/research_io.cpp
    src/core/planet_query_io.cpp
    src/core/heap_io.cpp
    src/core/production_io.cpp
    src/core/turn_io.cpp
    src/core/presentation_io.cpp
    src/core/fleet_query_io.cpp
    src/core/colony_io.cpp
)
target_link_libraries(stars_core PUBLIC stars_options stars_encoding stars_filesystem "$<$<PLATFORM_ID:Windows>:shell32>")

# The command-line host: the game code and src/host/host.cpp, no windows.
add_executable(stars-host src/host/host.cpp src/host/hostui.cpp)
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
# Python generators and test drivers share a build-local environment. Keep
# system site packages visible for the Linux D-Bus/GLib test drivers, which
# use the OS-provided Python bindings.
find_package(Python3 3.12 REQUIRED COMPONENTS Interpreter)

set(STARS_PYTHON_VENV "${CMAKE_BINARY_DIR}/python-venv")
if(WIN32)
    set(STARS_PYTHON_VENV_SCRIPTS_EXECUTABLE "${STARS_PYTHON_VENV}/Scripts/python.exe")
    set(STARS_PYTHON_VENV_BIN_EXECUTABLE "${STARS_PYTHON_VENV}/bin/python.exe")
else()
    set(STARS_PYTHON_VENV_SCRIPTS_EXECUTABLE "${STARS_PYTHON_VENV}/bin/python")
    set(STARS_PYTHON_VENV_BIN_EXECUTABLE "${STARS_PYTHON_VENV}/bin/python")
endif()
if(NOT EXISTS "${STARS_PYTHON_VENV_SCRIPTS_EXECUTABLE}" AND NOT EXISTS "${STARS_PYTHON_VENV_BIN_EXECUTABLE}")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m venv --system-site-packages --without-pip "${STARS_PYTHON_VENV}"
        RESULT_VARIABLE STARS_VENV_FAILED
        OUTPUT_VARIABLE STARS_VENV_OUTPUT
        ERROR_VARIABLE STARS_VENV_ERROR
    )
    if(STARS_VENV_FAILED)
        message(FATAL_ERROR "Could not create the Python build environment.\n${STARS_VENV_OUTPUT}${STARS_VENV_ERROR}")
    endif()
endif()
if(EXISTS "${STARS_PYTHON_VENV_SCRIPTS_EXECUTABLE}")
    set(STARS_PYTHON_VENV_EXECUTABLE "${STARS_PYTHON_VENV_SCRIPTS_EXECUTABLE}")
elseif(EXISTS "${STARS_PYTHON_VENV_BIN_EXECUTABLE}")
    set(STARS_PYTHON_VENV_EXECUTABLE "${STARS_PYTHON_VENV_BIN_EXECUTABLE}")
else()
    message(FATAL_ERROR "Python created ${STARS_PYTHON_VENV}, but its interpreter was not found in Scripts or bin.")
endif()
execute_process(
    COMMAND "${STARS_PYTHON_VENV_EXECUTABLE}" -m ensurepip --upgrade
    RESULT_VARIABLE STARS_ENSUREPIP_FAILED
    OUTPUT_VARIABLE STARS_ENSUREPIP_OUTPUT
    ERROR_VARIABLE STARS_ENSUREPIP_ERROR
)
if(STARS_ENSUREPIP_FAILED)
    message(FATAL_ERROR "Could not bootstrap pip in ${STARS_PYTHON_VENV}. Check that Python includes venv and ensurepip support.\n${STARS_ENSUREPIP_OUTPUT}${STARS_ENSUREPIP_ERROR}")
endif()
execute_process(
    COMMAND "${STARS_PYTHON_VENV_EXECUTABLE}" -m pip install --disable-pip-version-check
        --requirement "${CMAKE_CURRENT_SOURCE_DIR}/requirements-build.txt"
    RESULT_VARIABLE STARS_PYTHON_PACKAGES_FAILED
    OUTPUT_VARIABLE STARS_PYTHON_PACKAGES_OUTPUT
    ERROR_VARIABLE STARS_PYTHON_PACKAGES_ERROR
)
if(STARS_PYTHON_PACKAGES_FAILED)
    message(FATAL_ERROR "Could not install the Python build requirements into ${STARS_PYTHON_VENV}. Check network access and pip.\n${STARS_PYTHON_PACKAGES_OUTPUT}${STARS_PYTHON_PACKAGES_ERROR}")
endif()
set(Python3_EXECUTABLE "${STARS_PYTHON_VENV_EXECUTABLE}")
set(STARS_TEXT_SOURCES text/strings.txt text/messages.txt text/tutorial.txt text/planets.txt)
list(TRANSFORM STARS_TEXT_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set(STARS_TEXT_OUTPUTS textids.h text_strings.cpp text_messages.cpp text_planets.cpp text_tutorial.cpp)
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
# starting design and AI recipe enums) and data_tables.cpp (the parts, races,
# default battle plans and AI recipes) from data/**/*.yaml.
file(GLOB_RECURSE STARS_DATA_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/data/*.yaml")
set(STARS_DATA_OUTPUTS "${STARS_GENERATED_DIR}/dataids.h" "${STARS_GENERATED_DIR}/data_tables.cpp")
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
    "${STARS_GENERATED_DIR}/text_strings.cpp"
    "${STARS_GENERATED_DIR}/text_messages.cpp"
    "${STARS_GENERATED_DIR}/text_planets.cpp"
    "${STARS_GENERATED_DIR}/data_tables.cpp"
)

# The game's headers are found for #include "..." only (-iquote): strings.h
# is the game's string table, and glibc's <string.h> includes the system
# <strings.h>. The resource compiler takes them as include directories.
foreach(STARS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/core" "${CMAKE_CURRENT_SOURCE_DIR}/src/ui" "${CMAKE_CURRENT_SOURCE_DIR}/src/platform" "${CMAKE_CURRENT_SOURCE_DIR}/res" "${STARS_GENERATED_DIR}")
    target_compile_options(stars_options INTERFACE "$<$<COMPILE_LANGUAGE:C,CXX>:SHELL:-iquote ${STARS_INCLUDE_DIR}>")
    target_include_directories(stars_options INTERFACE "$<$<COMPILE_LANGUAGE:RC>:${STARS_INCLUDE_DIR}>")
endforeach()

# Match the existing MinGW compile-analysis flags. Plain char is signed, as
# in the original MSC build; the decompiler relies on it to drop char casts.
target_compile_options(stars_options INTERFACE
    $<$<COMPILE_LANGUAGE:C>:-Wno-pointer-sign>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fsigned-char>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fno-fast-math>
    $<$<COMPILE_LANGUAGE:C,CXX>:-ffp-contract=off>
)

# Binary64 operation boundaries are intentional; see docs/DOUBLE-PRECISION.md.
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
    #include <float.h>
    #if DBL_MANT_DIG != 53 || DBL_MAX_EXP != 1024 || FLT_EVAL_METHOD != 0
    #error unsupported double evaluation
    #endif
    int main(void) { return 0; }" STARS_BINARY64_DOUBLE_CXX20)
if(NOT STARS_BINARY64_DOUBLE_CXX20)
    message(FATAL_ERROR "Stars requires binary64 double without excess evaluation precision")
endif()

# Probe the language/library facilities used by the native interfaces.
check_cxx_source_compiles("
    #include <span>
    #include <bit>
    #include <cstdint>
    struct Probe { int i; };
    constinit Probe probe{.i = 20};
    int main() {
        int values[1] = {probe.i};
        std::span<int> view(values);
        return std::bit_cast<std::uint32_t>(1.0f) == 0 || view.size() != 1;
    }" STARS_CXX20_INTERFACES)
if(NOT STARS_CXX20_INTERFACES)
    message(FATAL_ERROR "Stars requires C++20 designated initializers, constinit, span and bit_cast")
endif()

target_link_libraries(stars_options INTERFACE m)
option(STARS_BUILD_TESTS "Build game and wrapper tests" ON)
option(STARS_BUILD_WIN32_REFERENCE_TESTS "Build independent Windows SDK contract tests" OFF)
if(STARS_BUILD_TESTS OR STARS_BUILD_WIN32_REFERENCE_TESTS)
    enable_testing()
    add_executable(stars-test-enum-abi tests/compat/enum_abi.cpp)
    target_link_libraries(stars-test-enum-abi PRIVATE stars_options)
    target_compile_options(stars-test-enum-abi PRIVATE -Wall -Wextra -Werror)
    add_test(NAME test_enum_abi COMMAND stars-test-enum-abi)
    set_tests_properties(test_enum_abi PROPERTIES LABELS build TIMEOUT 30)
    add_executable(stars-test-fileio tests/compat/fileio.cpp)
    target_link_libraries(stars-test-fileio PRIVATE stars_filesystem)
    target_compile_options(stars-test-fileio PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME test_fileio_contract COMMAND stars-test-fileio)
    set_tests_properties(test_fileio_contract PROPERTIES TIMEOUT 30)
endif()
if(STARS_BUILD_TESTS)
    add_executable(stars-test-native-publication tests/compat/native_publication.cpp)
    target_link_libraries(stars-test-native-publication PRIVATE stars_filesystem)
    target_compile_options(stars-test-native-publication PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
    add_test(NAME test_native_publication COMMAND stars-test-native-publication)
    set_tests_properties(test_native_publication PROPERTIES LABELS "ownership;lifetime" TIMEOUT 30)
    add_executable(stars-test-encoding-views tests/compat/encoding_views.cpp)
    target_link_libraries(stars-test-encoding-views PRIVATE stars_encoding stars_sanitizers)
    target_compile_options(stars-test-encoding-views PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME test_encoding_views COMMAND stars-test-encoding-views)
    set_tests_properties(test_encoding_views PROPERTIES LABELS "build;encoding" TIMEOUT 30)
    add_executable(stars-test-wire-codecs tests/compat/wire_codecs.cpp tests/compat/popup_payload_reference.c)
    target_link_libraries(stars-test-wire-codecs PRIVATE stars_options)
    target_compile_options(stars-test-wire-codecs PRIVATE -Wall -Wextra -Werror)
    add_test(NAME test_wire_codecs COMMAND stars-test-wire-codecs)
    set_tests_properties(test_wire_codecs PROPERTIES LABELS "build;wire" TIMEOUT 30)
    add_executable(stars-test-wire-codecs-natural tests/compat/wire_codecs.cpp tests/compat/popup_payload_reference.c)
    target_link_libraries(stars-test-wire-codecs-natural PRIVATE stars_options)
    target_compile_definitions(stars-test-wire-codecs-natural PRIVATE STARS_TEST_NATURAL_RECORD_LAYOUT=1)
    target_compile_options(stars-test-wire-codecs-natural PRIVATE -Wall -Wextra -Werror)
    add_test(NAME test_wire_codecs_natural COMMAND stars-test-wire-codecs-natural)
    set_tests_properties(test_wire_codecs_natural PROPERTIES LABELS "build;wire" TIMEOUT 30)
    add_test(NAME test_wire_inventory COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/cmake/check_wire_inventory.py"
        "${PROJECT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/wire-inventory.json")
    set_tests_properties(test_wire_inventory PROPERTIES LABELS "build;wire" TIMEOUT 30)
    add_executable(stars-catalog-dump tests/compat/catalog_dump.cpp)
    target_link_libraries(stars-catalog-dump PRIVATE stars_options)
    add_test(NAME test_catalog_bytes COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/check_catalog.py" "$<TARGET_FILE:stars-catalog-dump>")
    set_tests_properties(test_catalog_bytes PROPERTIES LABELS build TIMEOUT 30)
    add_executable(stars-layout-c tests/compat/record_layout.c)
    add_executable(stars-layout-cxx tests/compat/record_layout.cpp)
    target_link_libraries(stars-layout-c PRIVATE stars_options)
    target_link_libraries(stars-layout-cxx PRIVATE stars_options)
    add_test(NAME test_record_layout COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/check_record_layout.py"
        "$<TARGET_FILE:stars-layout-c>" "$<TARGET_FILE:stars-layout-cxx>")
    set_tests_properties(test_record_layout PROPERTIES LABELS "build" TIMEOUT 30)
endif()

if(STARS_BUILD_SDL_CLIENT)
    if(STARS_BUILD_TESTS)
        add_executable(stars-test-filesystem tests/compat/filesystem.cpp)
        target_link_libraries(stars-test-filesystem PRIVATE stars_filesystem)
        target_compile_options(stars-test-filesystem PRIVATE -Wall -Wextra -Wpedantic -Werror)
        add_test(NAME win32_utf8_filesystem COMMAND stars-test-filesystem)
        set_tests_properties(win32_utf8_filesystem PROPERTIES LABELS compat TIMEOUT 30)
    endif()
    include(cmake/sdl-client.cmake)
elseif(STARS_BUILD_TESTS)
    set(STARS_TEST_DIR "${CMAKE_BINARY_DIR}/tests")
    file(MAKE_DIRECTORY "${STARS_TEST_DIR}/data")
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/scaffold/utf8_fixtures.py"
        "${PROJECT_SOURCE_DIR}/tests/scaffold/fixtures/newgame/tiny/humanoid.r1"
        "${STARS_TEST_DIR}/data/humanoid.r1" COMMAND_ERROR_IS_FATAL ANY)
    file(GLOB STARS_UNIT_TESTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/test_*.cpp")
    # These tests need the desktop UI and its private file adapters.
    list(REMOVE_ITEM STARS_UNIT_TESTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/test_native_ports.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/test_native_fileui.cpp")
    set(STARS_HOST_TEST_TARGETS)
    foreach(STARS_UNIT_TEST IN LISTS STARS_UNIT_TESTS)
        get_filename_component(STARS_TEST_NAME "${STARS_UNIT_TEST}" NAME_WE)
        add_executable(${STARS_TEST_NAME} "${STARS_UNIT_TEST}" tests/unit/stars_test.cpp src/host/hostui.cpp)
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
    include(cmake/win32-reference-tests.cmake)
endif()

include(cmake/strict-cpp.cmake)
include(cmake/cpp-interface-checks.cmake)
