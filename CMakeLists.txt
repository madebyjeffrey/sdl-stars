cmake_minimum_required(VERSION 3.23)
# The version this line is building toward. Tags (vX.Y.Z) and build numbers
# come from git; see cmake/version.cmake and docs/VERSIONING.md.
project(stars VERSION 2.9.0 LANGUAGES C)

# The MinGW presets build the Windows game (stars.exe), stars-host.exe and
# the unit tests. Native presets build stars-host; the SDL presets also build
# the desktop compatibility smoke test.
if(WIN32)
    if(NOT MINGW)
        message(FATAL_ERROR "On Windows, configure using the MinGW toolchain: cmake --preset mingw-debug or cmake --preset mingw-release")
    endif()
    enable_language(RC)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(STARS_BUILD_SDL_CLIENT "Build the SDL3 desktop compatibility targets" OFF)
if(STARS_BUILD_SDL_CLIENT AND NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|Darwin)$")
    message(FATAL_ERROR "The SDL compatibility targets require native Linux or macOS")
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
# here only if the command-line host needs it; Windows dialogs and drawing go
# in STARS_UI_SOURCES. Some of these files still hold UI functions; the 2.9
# split moves those out (docs/ROADMAP.md).
set(STARS_CORE_SOURCES
    ai.c ai2.c ai3.c ai4.c aiutil.c battle.c build.c create.c file.c globals.c
    log.c memory.c mine.c msg.c native.c parts.c planet.c produce.c race.c
    report.c research.c save.c ship.c ship2.c strings.c
    thing.c turn.c turn2.c turn3.c util.c utilgen.c
)

# The Windows game's user interface.
set(STARS_UI_SOURCES
    battleui.c buildui.c createui.c fileui.c globalsui.c init.c logui.c mdi.c
    mineui.c msgui.c nativeui.c planetui.c popup.c produceui.c raceui.c
    reportui.c researchui.c scan.c scaninspect.c ship2ui.c shipui.c stars.c tb.c thingui.c
    tutor.c tutor2.c utilgenui.c utilui.c vcr.c
)

# Settings every target that compiles game code shares.
add_library(stars_options INTERFACE)
target_link_libraries(stars_options INTERFACE stars_sanitizers)

# Each set is built once for stars.exe and the unit tests.
add_library(stars_core OBJECT ${STARS_CORE_SOURCES})
target_link_libraries(stars_core PUBLIC stars_options)

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
)

if(NOT WIN32)
    target_link_libraries(stars_options INTERFACE m)
    # Turn generation rounds through long double as the original's x87 code
    # did (docs/NATIVE-PORT.md). Without x87 extended precision (a 64-bit
    # mantissa), as on Apple silicon or other ARM, some turns differ from
    # the Windows game's.
    include(CheckCSourceCompiles)
    check_c_source_compiles("
        #include <float.h>
        #if LDBL_MANT_DIG != 64
        #error not x87 extended precision
        #endif
        int main(void) { return 0; }" STARS_X87_LONG_DOUBLE)
    if(NOT STARS_X87_LONG_DOUBLE)
        message(WARNING "long double is not x87 extended precision here, so stars-host's turns can differ "
                        "from the Windows game's. On Apple silicon, use the macos-host presets (x86_64, run by Rosetta).")
    endif()
    # Unit tests (tests/unit) that need no windows, with hostui.c for the
    # user interface. The Windows build below runs them all under Wine.
    option(STARS_BUILD_TESTS "Build the unit tests in tests/unit" ON)
    if(STARS_BUILD_TESTS)
        enable_testing()
        set(STARS_TEST_DIR "${CMAKE_BINARY_DIR}/tests")
        file(MAKE_DIRECTORY "${STARS_TEST_DIR}/data")
        configure_file(tests/scaffold/fixtures/newgame/tiny/humanoid.r1 "${STARS_TEST_DIR}/data/humanoid.r1" COPYONLY)
        file(GLOB STARS_UNIT_TESTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/test_*.c")
        # test_native_ports replaces the game's file I/O with --wrap, which
        # only GNU ld on Windows provides.
        list(REMOVE_ITEM STARS_UNIT_TESTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/test_native_ports.c")
        set(STARS_HOST_TEST_TARGETS)
        foreach(STARS_UNIT_TEST ${STARS_UNIT_TESTS})
            get_filename_component(STARS_TEST_NAME "${STARS_UNIT_TEST}" NAME_WE)
            list(APPEND STARS_HOST_TEST_TARGETS "${STARS_TEST_NAME}")
            add_executable(${STARS_TEST_NAME} "${STARS_UNIT_TEST}" tests/unit/stars_test.c hostui.c)
            target_link_libraries(${STARS_TEST_NAME} PRIVATE stars_core)
            target_include_directories(${STARS_TEST_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit")
            target_compile_definitions(${STARS_TEST_NAME} PRIVATE STARS_TEST_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/golden")
            set_target_properties(${STARS_TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
            add_test(NAME ${STARS_TEST_NAME} COMMAND ${STARS_TEST_NAME})
            set_tests_properties(${STARS_TEST_NAME} PROPERTIES WORKING_DIRECTORY "${STARS_TEST_DIR}")
        endforeach()
        add_custom_target(stars-host-unit-tests DEPENDS ${STARS_HOST_TEST_TARGETS})
    endif()
    if(STARS_BUILD_SDL_CLIENT)
        include(cmake/sdl-client.cmake)
    endif()
    # The rest builds the Windows game and its tests.
    return()
endif()

add_library(stars_ui OBJECT ${STARS_UI_SOURCES} "${STARS_GENERATED_DIR}/text_tutorial.c")
target_link_libraries(stars_ui PUBLIC stars_options)
target_link_libraries(stars_ui PUBLIC user32 gdi32 comdlg32 comctl32)
add_dependencies(stars_ui stars_version)

add_executable(stars WIN32 res/stars.rc)
target_link_libraries(stars PRIVATE stars_core stars_ui)
set_target_properties(stars PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# MinGW runtime objects can contain debug data even in a Release build.
target_link_options(stars PRIVATE
    $<$<CONFIG:Release>:LINKER:--strip-debug>
)

# The tutorial runner reads this observer to drive real UI controls.
option(STARS_TEST_TUTORIAL "Expose read-only tutorial state for AutoHotkey tests" OFF)
if(STARS_TEST_TUTORIAL)
    target_sources(stars PRIVATE tests/scaffold/tutorial/observer.c)
    target_link_options(stars PRIVATE "LINKER:--wrap=InitInstance,--wrap=ScannerWndProc")
endif()

# Trace builds wrap cross-file calls and log when STARS_TRACE names a file.
option(STARS_TEST_TRACE "Log Random and other selected calls to $STARS_TRACE for regression tests" OFF)
if(STARS_TEST_TRACE)
    target_sources(stars PRIVATE tests/scaffold/regression_trace.c)
    target_link_options(stars PRIVATE "LINKER:--wrap=Random,--wrap=PctPlanetCapacity")
endif()

# Recompile resources when an embedded bitmap, icon, cursor, or data file changes.
file(GLOB_RECURSE STARS_RESOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/res/*"
)
list(REMOVE_ITEM STARS_RESOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/res/stars.rc"
)
set_source_files_properties(res/stars.rc PROPERTIES
    OBJECT_DEPENDS "${STARS_RESOURCE_FILES};${STARS_GENERATED_DIR}/version.h"
)

# Wine is needed only to run the Windows executable on the build host.
find_program(WINE_EXECUTABLE NAMES wine)
if(WINE_EXECUTABLE)
    set_property(TARGET stars PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    add_custom_target(run_in_wine
        COMMAND "${WINE_EXECUTABLE}" "$<TARGET_FILE:stars>"
        DEPENDS stars
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        USES_TERMINAL
        VERBATIM
    )
else()
    message(STATUS "Wine not found; stars.exe can still be built, but run_in_wine is unavailable")
endif()

# Unit tests (tests/unit): one acutest executable per test_*.c, linked with
# the game code and its resources. They are Windows console programs; CTest
# runs them through Wine when it is available.
option(STARS_BUILD_TESTS "Build the unit tests in tests/unit" ON)
if(STARS_BUILD_TESTS)
    enable_testing()
    set(STARS_TEST_DIR "${CMAKE_BINARY_DIR}/tests")
    # Use the SDK as an independent reference for the Linux UI ABI assertions.
    add_executable(test_win32_abi_reference tests/compat/win32_abi.c)
    target_link_libraries(test_win32_abi_reference PRIVATE stars_options)
    set_target_properties(test_win32_abi_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_abi_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_abi_reference COMMAND test_win32_abi_reference)
    set_tests_properties(test_win32_abi_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")
    add_executable(test_win32_toolbar_reference tests/compat/win32_toolbar.c)
    target_link_libraries(test_win32_toolbar_reference PRIVATE stars_options user32 gdi32 comctl32)
    set_target_properties(test_win32_toolbar_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_toolbar_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_toolbar_reference COMMAND test_win32_toolbar_reference)
    set_tests_properties(test_win32_toolbar_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all" TIMEOUT 30)
    # The SDK reference embeds the same RC assets, independently of the Linux loader.
    set(STARS_BITMAP_REFERENCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/compat-resources")
    file(GLOB STARS_BITMAP_REFERENCE_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/res/bitmaps/*.bmp")
    add_custom_command(OUTPUT "${STARS_BITMAP_REFERENCE_DIR}/bitmap_manifest.h"
        BYPRODUCTS "${STARS_BITMAP_REFERENCE_DIR}/bitmap_resources.c"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/compat/win32/resourcegen.py"
            "${CMAKE_SOURCE_DIR}" "${STARS_BITMAP_REFERENCE_DIR}"
        DEPENDS compat/win32/resourcegen.py res/stars.rc res/resource.h ${STARS_BITMAP_REFERENCE_FILES}
        VERBATIM)
    add_custom_target(stars_bitmap_reference_manifest DEPENDS "${STARS_BITMAP_REFERENCE_DIR}/bitmap_manifest.h")
    add_executable(test_win32_dib_reference tests/compat/win32_dib.c res/stars.rc)
    target_link_libraries(test_win32_dib_reference PRIVATE stars_options user32 gdi32)
    target_include_directories(test_win32_dib_reference PRIVATE "${STARS_BITMAP_REFERENCE_DIR}")
    add_dependencies(test_win32_dib_reference stars_bitmap_reference_manifest stars_version)
    set_target_properties(test_win32_dib_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_dib_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_dib_reference COMMAND test_win32_dib_reference)
    set_tests_properties(test_win32_dib_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")
    add_executable(test_win32_desktop_reference tests/compat/win32_desktop.c)
    target_link_libraries(test_win32_desktop_reference PRIVATE user32)
    set_target_properties(test_win32_desktop_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_desktop_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_desktop_reference COMMAND test_win32_desktop_reference)
    set_tests_properties(test_win32_desktop_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")

    add_executable(test_win32_list_reference tests/compat/win32_list.c)
    target_link_libraries(test_win32_list_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_list_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_list_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_list_reference COMMAND test_win32_list_reference)
    set_tests_properties(test_win32_list_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")

    add_executable(test_win32_edit_reference tests/compat/win32_edit.c)
    target_link_libraries(test_win32_edit_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_edit_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_edit_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_edit_reference COMMAND test_win32_edit_reference)
    set_tests_properties(test_win32_edit_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")

    add_executable(test_win32_controls_reference tests/compat/win32_controls.c)
    target_link_libraries(test_win32_controls_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_controls_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_controls_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_controls_reference COMMAND test_win32_controls_reference)
    set_tests_properties(test_win32_controls_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")

    add_executable(test_win32_scroll_reference tests/compat/win32_scroll.c)
    target_link_libraries(test_win32_scroll_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_scroll_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_scroll_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_scroll_reference COMMAND test_win32_scroll_reference)
    set_tests_properties(test_win32_scroll_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")


    add_custom_command(OUTPUT "${STARS_BITMAP_REFERENCE_DIR}/ui_manifest.h"
        BYPRODUCTS "${STARS_BITMAP_REFERENCE_DIR}/ui_resources.c"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/compat/win32/uigen.py"
            "${CMAKE_SOURCE_DIR}" "${STARS_BITMAP_REFERENCE_DIR}"
        DEPENDS compat/win32/uigen.py res/stars.rc res/resource.h compat/win32/include/wincompat_constants.h
            res/data/idr_tutorial_hst.bin res/data/idr_tutorial_m1.bin res/data/idr_tutorial_m2.bin
        VERBATIM)
    add_custom_target(stars_ui_reference_manifest DEPENDS "${STARS_BITMAP_REFERENCE_DIR}/ui_manifest.h")
    add_executable(test_win32_ui_resources_reference tests/compat/win32_ui_resources.c res/stars.rc)
    target_link_libraries(test_win32_ui_resources_reference PRIVATE stars_options user32 gdi32)
    target_include_directories(test_win32_ui_resources_reference PRIVATE "${STARS_BITMAP_REFERENCE_DIR}")
    add_dependencies(test_win32_ui_resources_reference stars_ui_reference_manifest stars_version)
    set_target_properties(test_win32_ui_resources_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_ui_resources_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_ui_resources_reference COMMAND test_win32_ui_resources_reference)
    set_tests_properties(test_win32_ui_resources_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all" TIMEOUT 30)

    add_executable(test_win32_popup_reference tests/compat/win32_popup.c)
    target_link_libraries(test_win32_popup_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_popup_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_popup_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_popup_reference COMMAND test_win32_popup_reference)
    set_tests_properties(test_win32_popup_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all" TIMEOUT 30)

    file(GLOB STARS_IMAGE_REFERENCE_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/res/icons/*.ico" "${CMAKE_SOURCE_DIR}/res/cursors/*.cur")
    add_custom_command(OUTPUT "${STARS_BITMAP_REFERENCE_DIR}/image_manifest.h"
        BYPRODUCTS "${STARS_BITMAP_REFERENCE_DIR}/image_resources.c"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/compat/win32/imagegen.py"
            "${CMAKE_SOURCE_DIR}" "${STARS_BITMAP_REFERENCE_DIR}"
        DEPENDS compat/win32/imagegen.py res/stars.rc res/resource.h ${STARS_IMAGE_REFERENCE_FILES}
        VERBATIM)
    add_custom_target(stars_image_reference_manifest DEPENDS "${STARS_BITMAP_REFERENCE_DIR}/image_manifest.h")
    add_executable(test_win32_filedialog_reference tests/compat/win32_filedialog.c)
    target_link_libraries(test_win32_filedialog_reference PRIVATE user32 gdi32 comdlg32)
    set_target_properties(test_win32_filedialog_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_filedialog_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_filedialog_reference COMMAND test_win32_filedialog_reference)
    set_tests_properties(test_win32_filedialog_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all" TIMEOUT 90)

    add_executable(test_win32_messagebox_reference tests/compat/win32_messagebox.c)
    target_link_libraries(test_win32_messagebox_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_messagebox_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_messagebox_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_messagebox_reference COMMAND test_win32_messagebox_reference)
    set_tests_properties(test_win32_messagebox_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all" TIMEOUT 90)

    add_executable(test_win32_palette_reference tests/compat/win32_palette.c)
    target_link_libraries(test_win32_palette_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_palette_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_palette_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_palette_reference COMMAND test_win32_palette_reference)
    set_tests_properties(test_win32_palette_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all" TIMEOUT 30)

    add_executable(test_win32_combo_reference tests/compat/win32_combo.c res/stars.rc)
    target_link_libraries(test_win32_combo_reference PRIVATE stars_options user32 gdi32)
    target_include_directories(test_win32_combo_reference PRIVATE "${STARS_BITMAP_REFERENCE_DIR}")
    add_dependencies(test_win32_combo_reference stars_version stars_ui_reference_manifest)
    set_target_properties(test_win32_combo_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_combo_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_combo_reference COMMAND test_win32_combo_reference)
    set_tests_properties(test_win32_combo_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all" TIMEOUT 120)

    add_executable(test_win32_dialog_reference tests/compat/win32_dialog.c res/stars.rc)
    target_link_libraries(test_win32_dialog_reference PRIVATE stars_options user32 gdi32)
    add_dependencies(test_win32_dialog_reference stars_version)
    set_target_properties(test_win32_dialog_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_dialog_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_dialog_reference COMMAND test_win32_dialog_reference)
    set_tests_properties(test_win32_dialog_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all" TIMEOUT 30)

    add_executable(test_win32_images_reference tests/compat/win32_images.c res/stars.rc)
    target_link_libraries(test_win32_images_reference PRIVATE stars_options user32 gdi32)
    target_include_directories(test_win32_images_reference PRIVATE "${STARS_BITMAP_REFERENCE_DIR}")
    add_dependencies(test_win32_images_reference stars_image_reference_manifest stars_version)
    set_target_properties(test_win32_images_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_images_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_images_reference COMMAND test_win32_images_reference)
    set_tests_properties(test_win32_images_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all" TIMEOUT 30)

    add_executable(test_win32_menubar_reference tests/compat/win32_menubar.c)
    target_link_libraries(test_win32_menubar_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_menubar_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_menubar_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_menubar_reference COMMAND test_win32_menubar_reference)
    set_tests_properties(test_win32_menubar_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all" TIMEOUT 30)

    add_executable(test_win32_menu_reference tests/compat/win32_menu.c)
    target_link_libraries(test_win32_menu_reference PRIVATE user32)
    set_target_properties(test_win32_menu_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_menu_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_menu_reference COMMAND test_win32_menu_reference)
    set_tests_properties(test_win32_menu_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")

    add_executable(test_win32_scrollwindow_reference tests/compat/win32_scrollwindow.c)
    target_link_libraries(test_win32_scrollwindow_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_scrollwindow_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_scrollwindow_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_scrollwindow_reference COMMAND test_win32_scrollwindow_reference)
    set_tests_properties(test_win32_scrollwindow_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")

    add_executable(test_win32_attention_reference tests/compat/win32_attention.c)
    target_link_libraries(test_win32_attention_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_attention_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_attention_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_attention_reference COMMAND test_win32_attention_reference)
    set_tests_properties(test_win32_attention_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")

    add_executable(test_win32_state_reference tests/compat/win32_state.c)
    target_link_libraries(test_win32_state_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_state_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_state_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_state_reference COMMAND test_win32_state_reference)
    set_tests_properties(test_win32_state_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")

    add_executable(test_win32_input_reference tests/compat/win32_input.c)
    target_link_libraries(test_win32_input_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_input_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_input_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_input_reference COMMAND test_win32_input_reference)
    set_tests_properties(test_win32_input_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")

    include(CheckCCompilerFlag)
    check_c_compiler_flag("-ftrivial-auto-var-init=pattern" STARS_C_PATTERN_INIT)
    add_executable(test_nativeui_points_reference tests/compat/nativeui_points.c)
    target_compile_options(test_nativeui_points_reference PRIVATE -Wall -Wextra -Wpedantic -Werror)
    if(STARS_C_PATTERN_INIT)
        target_compile_options(test_nativeui_points_reference PRIVATE -ftrivial-auto-var-init=pattern)
    endif()
    set_target_properties(test_nativeui_points_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_nativeui_points_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_nativeui_points_reference COMMAND test_nativeui_points_reference)
    set_tests_properties(test_nativeui_points_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")

    add_executable(test_win32_capture_reference tests/compat/win32_capture.c)
    target_link_libraries(test_win32_capture_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_capture_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_capture_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_capture_reference COMMAND test_win32_capture_reference)
    set_tests_properties(test_win32_capture_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")

    add_executable(test_win32_position_reference tests/compat/win32_position.c)
    target_link_libraries(test_win32_position_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_position_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_position_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_position_reference COMMAND test_win32_position_reference)
    set_tests_properties(test_win32_position_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")

    add_executable(test_win32_children_reference tests/compat/win32_children.c)
    target_link_libraries(test_win32_children_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_children_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_children_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_children_reference COMMAND test_win32_children_reference)
    set_tests_properties(test_win32_children_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")

    add_executable(test_win32_geometry_reference tests/compat/win32_geometry.c)
    target_link_libraries(test_win32_geometry_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_geometry_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_geometry_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_geometry_reference COMMAND test_win32_geometry_reference)
    set_tests_properties(test_win32_geometry_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")

    add_executable(test_win32_drawing_state_reference tests/compat/win32_drawing_state.c)
    target_link_libraries(test_win32_drawing_state_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_drawing_state_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_drawing_state_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_drawing_state_reference COMMAND test_win32_drawing_state_reference)
    set_tests_properties(test_win32_drawing_state_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")

    add_executable(test_win32_regions_reference tests/compat/win32_regions.c)
    target_link_libraries(test_win32_regions_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_regions_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_regions_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_regions_reference COMMAND test_win32_regions_reference)
    set_tests_properties(test_win32_regions_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")

    add_executable(test_win32_font_reference tests/compat/win32_font.c)
    target_link_libraries(test_win32_font_reference PRIVATE user32 gdi32)
    set_target_properties(test_win32_font_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_font_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_font_reference COMMAND test_win32_font_reference)
    set_tests_properties(test_win32_font_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")
    add_executable(test_win32_file_reference tests/compat/win32_file.c)
    set_target_properties(test_win32_file_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_file_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_file_reference COMMAND test_win32_file_reference)
    set_tests_properties(test_win32_file_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")
    add_executable(test_win32_profile_reference tests/compat/win32_profile.c)
    set_target_properties(test_win32_profile_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_profile_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_profile_reference COMMAND test_win32_profile_reference)
    set_tests_properties(test_win32_profile_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")
    add_executable(test_win32_services_reference tests/compat/win32_services.c)
    target_link_libraries(test_win32_services_reference PRIVATE user32)
    set_target_properties(test_win32_services_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_services_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_services_reference COMMAND test_win32_services_reference)
    set_tests_properties(test_win32_services_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")
    add_executable(test_win32_memory_reference tests/compat/win32_memory.c)
    set_target_properties(test_win32_memory_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_memory_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_memory_reference COMMAND test_win32_memory_reference)
    set_tests_properties(test_win32_memory_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")
    add_executable(test_win32_timers_reference tests/compat/win32_timers.c)
    target_link_libraries(test_win32_timers_reference PRIVATE user32)
    set_target_properties(test_win32_timers_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_timers_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_timers_reference COMMAND test_win32_timers_reference)
    set_tests_properties(test_win32_timers_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all" TIMEOUT 30)
    add_executable(test_win32_bitmap_reference tests/compat/win32_bitmap.c)
    set_target_properties(test_win32_bitmap_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    target_link_libraries(test_win32_bitmap_reference PRIVATE user32 gdi32)
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_bitmap_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_bitmap_reference COMMAND test_win32_bitmap_reference)
    set_tests_properties(test_win32_bitmap_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")
    add_executable(test_win32_gdi_reference tests/compat/win32_gdi_reference.c)
    set_target_properties(test_win32_gdi_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    target_link_libraries(test_win32_gdi_reference PRIVATE user32 gdi32)
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_gdi_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_gdi_reference COMMAND test_win32_gdi_reference)
    set_tests_properties(test_win32_gdi_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")
    add_executable(test_win32_runtime_reference tests/compat/win32_runtime.c)
    set_target_properties(test_win32_runtime_reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    target_link_libraries(test_win32_runtime_reference PRIVATE user32)
    if(WINE_EXECUTABLE)
        set_property(TARGET test_win32_runtime_reference PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    endif()
    add_test(NAME test_win32_runtime_reference COMMAND test_win32_runtime_reference)
    set_tests_properties(test_win32_runtime_reference PROPERTIES LABELS "compat" ENVIRONMENT "WINEDEBUG=-all")
    file(MAKE_DIRECTORY "${STARS_TEST_DIR}/data")
    configure_file(tests/scaffold/fixtures/newgame/tiny/humanoid.r1 "${STARS_TEST_DIR}/data/humanoid.r1" COPYONLY)
    file(GLOB STARS_UNIT_TESTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/test_*.c")
    foreach(STARS_UNIT_TEST ${STARS_UNIT_TESTS})
        get_filename_component(STARS_TEST_NAME "${STARS_UNIT_TEST}" NAME_WE)
        add_executable(${STARS_TEST_NAME} "${STARS_UNIT_TEST}" tests/unit/stars_test.c res/stars.rc)
        target_link_libraries(${STARS_TEST_NAME} PRIVATE stars_core stars_ui)
        # Message boxes would wait for a click; stars_test.c records them.
        target_link_options(${STARS_TEST_NAME} PRIVATE "LINKER:--wrap=AlertSz")
        target_include_directories(${STARS_TEST_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit")
        target_compile_definitions(${STARS_TEST_NAME} PRIVATE STARS_TEST_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/golden")
        set_target_properties(${STARS_TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
        if(WINE_EXECUTABLE)
            set_property(TARGET ${STARS_TEST_NAME} PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
        endif()
        add_test(NAME ${STARS_TEST_NAME} COMMAND ${STARS_TEST_NAME})
        set_tests_properties(${STARS_TEST_NAME} PROPERTIES
            WORKING_DIRECTORY "${STARS_TEST_DIR}"
            ENVIRONMENT "WINEDEBUG=-all")
    endforeach()
    # Games to open by hand (tests/scenarios, make scenario).
    add_executable(stars_scenario tests/scenarios/scenarios.c tests/unit/stars_test.c res/stars.rc)
    target_link_libraries(stars_scenario PRIVATE stars_core stars_ui)
    target_link_options(stars_scenario PRIVATE "LINKER:--wrap=AlertSz")
    target_include_directories(stars_scenario PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit")
    set_target_properties(stars_scenario PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
    # The native-port test replaces file I/O with in-memory records.
    target_link_options(test_native_ports PRIVATE
        "LINKER:--wrap=WriteRt,--wrap=ReadRt,--wrap=FCreateFile,--wrap=StreamOpen,--wrap=StreamClose,--wrap=DirtyGame")
endif()
