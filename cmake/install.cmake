# Product-only layout. Resources are located relative to the executable so a
# staged installation can move without retaining paths into the source tree.
include(GNUInstallDirs)
if(WIN32 AND STARS_BUILD_SDL_CLIENT)
    set(STARS_RUNTIME_DEPENDENCIES RUNTIME_DEPENDENCY_SET StarsWindowsRuntime)
endif()
install(TARGETS stars-host ${STARS_RUNTIME_DEPENDENCIES} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT StarsRuntime)
if(NOT STARS_BUILD_SDL_CLIENT)
    return()
endif()
file(RELATIVE_PATH STARS_DATA_FROM_BIN "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}"
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/stars")
if(APPLE AND STARS_MACOS_BUNDLE)
    # SDL_GetBasePath returns Contents/Resources for a Cocoa app bundle.
    set(STARS_DATA_FROM_BIN ".")
endif()
target_compile_definitions(stars_win32 PRIVATE STARS_INSTALLED_DATA="${STARS_DATA_FROM_BIN}")
file(RELATIVE_PATH STARS_LIB_FROM_BIN "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}"
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_target_properties(stars-sdl stars-compat-help-viewer PROPERTIES
        INSTALL_RPATH "$ORIGIN/${STARS_LIB_FROM_BIN}")
endif()
install(TARGETS stars-sdl stars-compat-help-viewer ${STARS_RUNTIME_DEPENDENCIES}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT StarsRuntime)
get_target_property(STARS_SDL_IMPORTED SDL3::SDL3-shared IMPORTED)
if(NOT STARS_SDL_IMPORTED)
    install(TARGETS SDL3-shared LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT StarsRuntime)
    install(FILES "${sdl3_SOURCE_DIR}/LICENSE.txt"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/stars/licenses/SDL3" COMPONENT StarsRuntime)
endif()
if(WIN32)
    install(RUNTIME_DEPENDENCY_SET StarsWindowsRuntime
        DIRECTORIES "$<TARGET_FILE_DIR:SDL3::SDL3-shared>"
        PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
        POST_EXCLUDE_REGEXES ".*[Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\].*"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT StarsRuntime)
endif()
install(DIRECTORY "${STARS_HELP_GENERATED}/" DESTINATION "${CMAKE_INSTALL_DATADIR}/stars/help"
    COMPONENT StarsRuntime PATTERN "*.h" EXCLUDE)
install(FILES "${PROJECT_SOURCE_DIR}/compat/win32/LICENSE"
    "${PROJECT_SOURCE_DIR}/compat/win32/LICENSE.free-api"
    "${PROJECT_SOURCE_DIR}/compat/win32/LICENSE.zingl"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/stars/licenses" COMPONENT StarsRuntime)
install(FILES "${PROJECT_SOURCE_DIR}/res/fonts/LICENSE" "${PROJECT_SOURCE_DIR}/res/fonts/ORIGIN.json"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/stars/licenses/fonts" COMPONENT StarsRuntime)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(FILES "${PROJECT_SOURCE_DIR}/packaging/org.starsgame.Stars.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications" COMPONENT StarsRuntime)
    install(FILES "${PROJECT_SOURCE_DIR}/packaging/org.starsgame.Stars.metainfo.xml"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo" COMPONENT StarsRuntime)
    install(FILES "${PROJECT_SOURCE_DIR}/packaging/org.starsgame.Stars.svg"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps" COMPONENT StarsRuntime)
    install(FILES "${PROJECT_SOURCE_DIR}/packaging/org.starsgame.Stars.png"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps" COMPONENT StarsRuntime)
    install(FILES "${PROJECT_SOURCE_DIR}/res/icons/stars-app.png"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps"
        RENAME org.starsgame.Stars.png COMPONENT StarsRuntime)
endif()
