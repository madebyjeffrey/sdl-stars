# Host-only builds never load SDL. Every desktop build uses this dependency.
option(STARS_SDL_FETCH "Download pinned SDL3 when no installed SDL3 package is found" OFF)
find_package(SDL3 3.4 CONFIG QUIET)
if(NOT SDL3_FOUND)
    if(NOT STARS_SDL_FETCH)
        message(FATAL_ERROR "SDL3 >= 3.4 is required. Set CMAKE_PREFIX_PATH to its install prefix, or enable STARS_SDL_FETCH (see docs/SDL3-DEVELOPMENT.md).")
    endif()
    include(FetchContent)
    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    set(SDL_SHARED ON CACHE BOOL "Build shared SDL3")
    set(SDL_STATIC OFF CACHE BOOL "Build static SDL3")
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build SDL test library")
    set(SDL_TESTS OFF CACHE BOOL "Build SDL test programs")
    set(SDL_INSTALL OFF CACHE BOOL "Stars installs only the SDL runtime")
    set(SDL_EXAMPLES OFF CACHE BOOL "Build SDL examples")
    FetchContent_Declare(SDL3
        URL https://www.libsdl.org/release/SDL3-3.4.18.tar.gz
        URL_HASH SHA256=9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3
    )
    FetchContent_MakeAvailable(SDL3)
    # Installed SDL targets already expose system headers. Give the fetched
    # dependency the same boundary for its C ABI macros and declarations.
    get_target_property(STARS_SDL_HEADERS SDL3::Headers ALIASED_TARGET)
    get_target_property(STARS_SDL_INCLUDE_DIRS ${STARS_SDL_HEADERS} INTERFACE_INCLUDE_DIRECTORIES)
    set_property(TARGET ${STARS_SDL_HEADERS} APPEND PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
        "${STARS_SDL_INCLUDE_DIRS}")
    message(STATUS "Stars SDL3 dependency: fetched 3.4.18")
else()
    message(STATUS "Stars SDL3 dependency: installed ${SDL3_VERSION}")
endif()
