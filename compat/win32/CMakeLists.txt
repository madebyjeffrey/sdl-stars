add_library(stars_win32_headers INTERFACE)
target_include_directories(stars_win32_headers BEFORE INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(stars_win32_headers INTERFACE STARS_UI_SDL=1)

if(WIN32)
    # Keep the source API names, but isolate symbols from the real Win32
    # imports used by SDL, Cairo and native.c. The wrapper uses the C ABI.
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/wincompat_api.h" STARS_COMPAT_API)
    string(REGEX MATCHALL "(WINAPI|WINAPIV)[ \t\r\n]+[A-Za-z0-9_]+[ \t]*\\(" STARS_COMPAT_DECLARATIONS "${STARS_COMPAT_API}")
    set(STARS_COMPAT_NAMESPACE "/* Generated from wincompat_api.h; do not edit. */\n#define WinMain StarsCompat_WinMain\n")
    foreach(STARS_COMPAT_DECLARATION IN LISTS STARS_COMPAT_DECLARATIONS)
        string(REGEX REPLACE "(WINAPI|WINAPIV)[ \t\r\n]+([A-Za-z0-9_]+)[ \t]*\\(" "\\2" STARS_COMPAT_NAME "${STARS_COMPAT_DECLARATION}")
        string(APPEND STARS_COMPAT_NAMESPACE "#define ${STARS_COMPAT_NAME} StarsCompat_${STARS_COMPAT_NAME}\n")
    endforeach()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/wincompat_api.h")
    file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/wincompat_namespace.h" CONTENT "${STARS_COMPAT_NAMESPACE}" @ONLY)
    target_include_directories(stars_win32_headers INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/include")
endif()

file(GLOB STARS_BITMAP_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/res/bitmaps/*.bmp")
set(STARS_BITMAP_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(STARS_BITMAP_GENERATED "${STARS_BITMAP_GENERATED}" PARENT_SCOPE)
add_custom_command(
    OUTPUT "${STARS_BITMAP_GENERATED}/bitmap_resources.c" "${STARS_BITMAP_GENERATED}/bitmap_manifest.h"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/resourcegen.py"
        "${PROJECT_SOURCE_DIR}" "${STARS_BITMAP_GENERATED}"
    DEPENDS resourcegen.py "${PROJECT_SOURCE_DIR}/res/stars.rc"
        "${PROJECT_SOURCE_DIR}/res/resource.h" ${STARS_BITMAP_FILES}
    VERBATIM)
add_custom_target(stars_bitmap_resources DEPENDS "${STARS_BITMAP_GENERATED}/bitmap_manifest.h")

file(GLOB STARS_FONT_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/res/fonts/*.ttf")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.zingl" "${CMAKE_BINARY_DIR}/bin/LICENSE.zingl" COPYONLY)
configure_file("${PROJECT_SOURCE_DIR}/res/fonts/LICENSE" "${CMAKE_BINARY_DIR}/bin/LICENSE.fonts" COPYONLY)
configure_file("${PROJECT_SOURCE_DIR}/res/fonts/ORIGIN.json" "${CMAKE_BINARY_DIR}/bin/FONTS-ORIGIN.json" COPYONLY)
add_custom_command(
    OUTPUT "${STARS_BITMAP_GENERATED}/font_resources.c"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/fontgen.py"
        "${PROJECT_SOURCE_DIR}/res/fonts" "${STARS_BITMAP_GENERATED}/font_resources.c"
    DEPENDS fontgen.py "${PROJECT_SOURCE_DIR}/res/fonts/ORIGIN.json" ${STARS_FONT_FILES}
    VERBATIM)

file(GLOB STARS_CUSTOM_RESOURCE_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/res/data/*.bin")
add_custom_command(
    OUTPUT "${STARS_BITMAP_GENERATED}/ui_resources.c" "${STARS_BITMAP_GENERATED}/ui_manifest.h"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/uigen.py"
        "${PROJECT_SOURCE_DIR}" "${STARS_BITMAP_GENERATED}"
    DEPENDS uigen.py "${PROJECT_SOURCE_DIR}/res/stars.rc" "${PROJECT_SOURCE_DIR}/res/resource.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/wincompat_constants.h" ${STARS_CUSTOM_RESOURCE_FILES}
    VERBATIM)

file(GLOB STARS_IMAGE_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/res/icons/*.ico" "${PROJECT_SOURCE_DIR}/res/cursors/*.cur")
add_custom_command(
    OUTPUT "${STARS_BITMAP_GENERATED}/image_resources.c" "${STARS_BITMAP_GENERATED}/image_manifest.h"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/imagegen.py" "${PROJECT_SOURCE_DIR}" "${STARS_BITMAP_GENERATED}"
    DEPENDS imagegen.py "${PROJECT_SOURCE_DIR}/res/stars.rc" "${PROJECT_SOURCE_DIR}/res/resource.h"
        "${PROJECT_SOURCE_DIR}/res/icons/stars-app.png" ${STARS_IMAGE_FILES}
    VERBATIM)

find_package(Freetype REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(STARS_FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)
pkg_check_modules(STARS_CAIRO REQUIRED IMPORTED_TARGET cairo>=1.16 cairo-pdf)
# Private test variants are created in the parent directory.
foreach(STARS_DEPENDENCY Freetype::Freetype ZLIB::ZLIB PkgConfig::STARS_FONTCONFIG PkgConfig::STARS_CAIRO)
    set_property(TARGET ${STARS_DEPENDENCY} PROPERTY IMPORTED_GLOBAL TRUE)
endforeach()

# Only adapted C11 implementations are built, never the upstream reference copy.
add_library(stars_win32 STATIC src/toolbar.c src/inspect.c src/session.c src/help.c src/document.c src/print.c src/filedialog.c src/audio.c src/messagebox.c src/combo.c src/dialog.c src/dialog_template.c src/image.c src/accelerator.c src/menu.c src/rect.c src/time.c src/window.c src/message.c src/input.c src/control.c src/edit.c src/list.c src/paint.c src/dib.c src/resource.c src/memory.c src/timer.c src/services.c src/system.c src/scroll.c src/profile.c src/file.c src/font.c "${STARS_BITMAP_GENERATED}/font_resources.c" "${STARS_BITMAP_GENERATED}/bitmap_resources.c" "${STARS_BITMAP_GENERATED}/ui_resources.c" "${STARS_BITMAP_GENERATED}/image_resources.c")
target_link_libraries(stars_win32 PUBLIC stars_win32_headers stars_sanitizers stars_encoding PRIVATE SDL3::SDL3 Freetype::Freetype ZLIB::ZLIB PkgConfig::STARS_FONTCONFIG PkgConfig::STARS_CAIRO m)
option(STARS_NATIVE_FILE_DIALOGS "Use platform-native file selection" ON)
if(STARS_NATIVE_FILE_DIALOGS AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    pkg_check_modules(STARS_DBUS REQUIRED IMPORTED_TARGET GLOBAL dbus-1)
    target_sources(stars_win32 PRIVATE src/filedialog_portal.c)
    target_link_libraries(stars_win32 PRIVATE PkgConfig::STARS_DBUS)
    target_compile_definitions(stars_win32 PRIVATE STARS_FILE_DIALOG_PORTAL=1)
elseif(STARS_NATIVE_FILE_DIALOGS AND APPLE)
    enable_language(OBJC)
    target_sources(stars_win32 PRIVATE src/filedialog_cocoa.c src/filedialog_cocoa.m)
    set_source_files_properties(src/filedialog_cocoa.m PROPERTIES COMPILE_OPTIONS "-fobjc-arc;-fblocks")
    target_link_libraries(stars_win32 PRIVATE "-framework AppKit")
    target_compile_definitions(stars_win32 PRIVATE STARS_FILE_DIALOG_COCOA=1)
elseif(STARS_NATIVE_FILE_DIALOGS AND WIN32)
    # The SDK bridge must never inherit the wrapper include directories.
    add_library(stars_filedialog_windows OBJECT src/filedialog_windows_sdk.c)
    target_compile_options(stars_filedialog_windows PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_sources(stars_win32 PRIVATE src/filedialog_windows.c $<TARGET_OBJECTS:stars_filedialog_windows>)
    target_link_libraries(stars_win32 PRIVATE comdlg32)
    target_compile_definitions(stars_win32 PRIVATE STARS_FILE_DIALOG_WINDOWS=1)
endif()

target_compile_options(stars_win32 PRIVATE -Wall -Wextra -Wpedantic -Werror)

if(WIN32)
    # This translation unit alone includes the actual SDK, never wrapper headers.
    add_library(stars_compat_platform OBJECT src/platform_windows.c)
    target_compile_definitions(stars_compat_platform PRIVATE _FILE_OFFSET_BITS=64)
    target_compile_definitions(stars_win32 PUBLIC _FILE_OFFSET_BITS=64)
    target_link_libraries(stars_win32 PRIVATE bcrypt)
    target_compile_options(stars_compat_platform PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_sources(stars_win32 PRIVATE $<TARGET_OBJECTS:stars_compat_platform>)
endif()

target_include_directories(stars_win32 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
