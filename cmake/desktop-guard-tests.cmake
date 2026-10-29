add_executable(stars-resource-guards tests/compat/guard_ownership.cpp)
target_include_directories(stars-resource-guards PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_link_libraries(stars-resource-guards PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-resource-guards PRIVATE -Wall -Wextra -Werror)
stars_test_redirect(stars-resource-guards CbReadNativeFile)
add_test(NAME win32_sdl_resource_guards COMMAND stars-resource-guards "${CMAKE_BINARY_DIR}/guard-file")
set_tests_properties(win32_sdl_resource_guards PROPERTIES LABELS "compat;ownership;lifetime"
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
if(APPLE AND NOT STARS_ENABLE_SANITIZERS)
    find_program(STARS_GUARD_LEAKS_EXECUTABLE leaks REQUIRED)
    add_test(NAME win32_sdl_leaks_guards COMMAND "${STARS_GUARD_LEAKS_EXECUTABLE}"
        --quiet --noContent --atExit -- "$<TARGET_FILE:stars-resource-guards>" "${CMAKE_BINARY_DIR}/guard-file-leaks")
    set_tests_properties(win32_sdl_leaks_guards PROPERTIES LABELS "compat;ownership;lifetime;leaks"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" RUN_SERIAL TRUE TIMEOUT 60)
endif()
