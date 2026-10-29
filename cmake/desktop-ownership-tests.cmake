add_executable(stars-resource-faults tests/compat/resource_ownership.cpp)
target_include_directories(stars-resource-faults PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_link_libraries(stars-resource-faults PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-resource-faults PRIVATE -Wall -Wextra -Werror)
stars_test_redirect(stars-resource-faults SDL_DestroySurface)
add_test(NAME win32_sdl_resource_faults COMMAND stars-resource-faults)
set_tests_properties(win32_sdl_resource_faults PROPERTIES LABELS "compat;ownership"
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
