add_executable(stars-resource-guards tests/compat/guard_ownership.cpp)
target_include_directories(stars-resource-guards PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_link_libraries(stars-resource-guards PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-resource-guards PRIVATE -Wall -Wextra -Werror)
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

add_executable(stars-profile-save tests/compat/profile_save.cpp)
target_include_directories(stars-profile-save PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_link_libraries(stars-profile-save PRIVATE stars_win32)
target_compile_options(stars-profile-save PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME win32_profile_save COMMAND stars-profile-save)
set_tests_properties(win32_profile_save PROPERTIES LABELS "compat;ownership;lifetime" TIMEOUT 30)

if(STARS_NATIVE_FILE_DIALOGS)
    add_executable(stars-resource-modal tests/compat/modal_ownership.cpp)
    target_include_directories(stars-resource-modal PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
    target_link_libraries(stars-resource-modal PRIVATE stars_win32 SDL3::SDL3)
    target_compile_options(stars-resource-modal PRIVATE -Wall -Wextra -Werror)

    add_test(NAME win32_sdl_resource_modal COMMAND stars-resource-modal)
    set_tests_properties(win32_sdl_resource_modal PROPERTIES LABELS "compat;ownership;lifetime"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=native" TIMEOUT 30)
    if(APPLE AND NOT STARS_ENABLE_SANITIZERS)
        add_test(NAME win32_sdl_leaks_modal COMMAND "${STARS_GUARD_LEAKS_EXECUTABLE}"
            --quiet --noContent --atExit -- "$<TARGET_FILE:stars-resource-modal>")
        set_tests_properties(win32_sdl_leaks_modal PROPERTIES LABELS "compat;ownership;lifetime;leaks"
            ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=native" RUN_SERIAL TRUE TIMEOUT 60)
    endif()
endif()

# Qualify notification-capable cleanup with the existing message-box and nested
# callback vectors on every desktop backend, including Windows SDL private cases.
add_executable(stars-resource-messagebox tests/compat/win32_messagebox.cpp)
target_include_directories(stars-resource-messagebox PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_link_libraries(stars-resource-messagebox PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-resource-messagebox PRIVATE -Wall -Wextra -Werror)

add_test(NAME win32_sdl_resource_messagebox COMMAND stars-resource-messagebox)
set_tests_properties(win32_sdl_resource_messagebox PROPERTIES LABELS "compat;ownership;lifetime"
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 60)
set_property(TEST win32_exception_callbacks win32_sdl_paint_batch APPEND PROPERTY LABELS ownership lifetime)
if(APPLE AND NOT STARS_ENABLE_SANITIZERS)
    foreach(STARS_MODAL_LEAK_KIND messagebox exception_callbacks paint_batch)
        if(STARS_MODAL_LEAK_KIND STREQUAL "messagebox")
            set(STARS_MODAL_LEAK_TARGET stars-resource-messagebox)
        elseif(STARS_MODAL_LEAK_KIND STREQUAL "paint_batch")
            set(STARS_MODAL_LEAK_TARGET stars-compat-paint-batch)
        else()
            set(STARS_MODAL_LEAK_TARGET stars-compat-exceptions)
        endif()
        add_test(NAME win32_sdl_leaks_${STARS_MODAL_LEAK_KIND} COMMAND "${STARS_GUARD_LEAKS_EXECUTABLE}"
            --quiet --noContent --atExit -- "$<TARGET_FILE:${STARS_MODAL_LEAK_TARGET}>")
        set_tests_properties(win32_sdl_leaks_${STARS_MODAL_LEAK_KIND} PROPERTIES LABELS "compat;ownership;lifetime;leaks"
            ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" RUN_SERIAL TRUE TIMEOUT 60)
    endforeach()
endif()

# Control state is retired immediately, while active callback frames retain its
# storage. Reuse behavioral/SDK contracts alongside explicit lifetime vectors.
foreach(STARS_CONTROL_KIND list combo toolbar profile)
    add_executable(stars-resource-${STARS_CONTROL_KIND} tests/compat/win32_${STARS_CONTROL_KIND}.cpp)
    target_include_directories(stars-resource-${STARS_CONTROL_KIND} PRIVATE
        "${PROJECT_SOURCE_DIR}/compat/win32/src" "${STARS_BITMAP_GENERATED}")
    target_link_libraries(stars-resource-${STARS_CONTROL_KIND} PRIVATE stars_win32 SDL3::SDL3)
    target_compile_options(stars-resource-${STARS_CONTROL_KIND} PRIVATE -Wall -Wextra -Werror)
    add_test(NAME win32_sdl_resource_${STARS_CONTROL_KIND} COMMAND stars-resource-${STARS_CONTROL_KIND})
    set_tests_properties(win32_sdl_resource_${STARS_CONTROL_KIND} PROPERTIES LABELS "compat;ownership;lifetime"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 60)
endforeach()
add_executable(stars-resource-controls tests/compat/control_ownership.cpp)
target_include_directories(stars-resource-controls PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_link_libraries(stars-resource-controls PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-resource-controls PRIVATE -Wall -Wextra -Werror)
add_test(NAME win32_sdl_resource_controls COMMAND stars-resource-controls)
set_tests_properties(win32_sdl_resource_controls PROPERTIES LABELS "compat;ownership;lifetime"
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
if(APPLE AND NOT STARS_ENABLE_SANITIZERS)
    foreach(STARS_CONTROL_KIND list combo toolbar controls profile)
        add_test(NAME win32_sdl_leaks_${STARS_CONTROL_KIND} COMMAND "${STARS_GUARD_LEAKS_EXECUTABLE}"
            --quiet --noContent --atExit -- "$<TARGET_FILE:stars-resource-${STARS_CONTROL_KIND}>")
        set_tests_properties(win32_sdl_leaks_${STARS_CONTROL_KIND} PROPERTIES LABELS "compat;ownership;lifetime;leaks"
            ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" RUN_SERIAL TRUE TIMEOUT 60)
    endforeach()
endif()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/ownership-profile")
set_tests_properties(win32_sdl_resource_profile PROPERTIES RUN_SERIAL TRUE
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/ownership-profile")
if(APPLE AND NOT STARS_ENABLE_SANITIZERS)
    set_tests_properties(win32_sdl_leaks_profile PROPERTIES
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/ownership-profile")
endif()

add_executable(stars-resource-messages tests/compat/message_ownership.cpp)
target_include_directories(stars-resource-messages PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_link_libraries(stars-resource-messages PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-resource-messages PRIVATE -Wall -Wextra -Werror)

add_test(NAME win32_sdl_resource_messages COMMAND stars-resource-messages)
set_tests_properties(win32_sdl_resource_messages PROPERTIES LABELS "compat;ownership;lifetime"
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
if(APPLE AND NOT STARS_ENABLE_SANITIZERS)
    add_test(NAME win32_sdl_leaks_messages COMMAND "${STARS_GUARD_LEAKS_EXECUTABLE}"
        --quiet --noContent --atExit -- "$<TARGET_FILE:stars-resource-messages>")
    set_tests_properties(win32_sdl_leaks_messages PROPERTIES LABELS "compat;ownership;lifetime;leaks"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" RUN_SERIAL TRUE TIMEOUT 60)
endif()
