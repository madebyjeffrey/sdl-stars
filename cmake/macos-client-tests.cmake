# Portable client workflows also qualify the macOS desktop.
add_executable(stars-compat-client-startup tests/compat/win32_client_startup.cpp)
target_link_libraries(stars-compat-client-startup PRIVATE stars_core stars-ui-compat-check
    stars_options stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-client-startup PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_definitions(stars-compat-client-startup PRIVATE STARS_TEST_CLIENT_STARTUP=1)
target_compile_options(stars-compat-client-startup PRIVATE -Wall -Wextra -Werror)

set_target_properties(stars-compat-client-startup PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME win32_client_continue_dummy COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tests/compat/run_client_continue.py" "$<TARGET_FILE:stars-compat-client-startup>")
set_tests_properties(win32_client_continue_dummy PROPERTIES LABELS client TIMEOUT 200
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat")


add_test(NAME win32_client_designer_dummy COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tests/compat/run_client_continue.py" "$<TARGET_FILE:stars-compat-client-startup>")
set_tests_properties(win32_client_designer_dummy PROPERTIES LABELS client TIMEOUT 200
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat;STARS_CLIENT_WORKFLOW=designer")

add_test(NAME win32_client_newgame_dummy COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tests/compat/run_newgame.py" "$<TARGET_FILE:stars-compat-client-startup>")
set_tests_properties(win32_client_newgame_dummy PROPERTIES LABELS client TIMEOUT 200
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat")

add_test(NAME win32_client_game_export_dummy COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tests/compat/run_game_export.py" "$<TARGET_FILE:stars-compat-client-startup>")
set_tests_properties(win32_client_game_export_dummy PROPERTIES LABELS client TIMEOUT 90
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat")

add_test(NAME win32_client_game_library_safety_dummy COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tests/compat/run_game_library_safety.py" "$<TARGET_FILE:stars-compat-client-startup>")
set_tests_properties(win32_client_game_library_safety_dummy PROPERTIES LABELS client TIMEOUT 180
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat")
add_test(NAME win32_client_game_library_delete_dummy COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tests/compat/run_game_library_delete.py" "$<TARGET_FILE:stars-compat-client-startup>")
set_tests_properties(win32_client_game_library_delete_dummy PROPERTIES LABELS "client;library" TIMEOUT 90
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat")

add_test(NAME win32_client_race_library_dummy COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tests/compat/run_racelibrary.py" "$<TARGET_FILE:stars-compat-client-startup>")
set_tests_properties(win32_client_race_library_dummy PROPERTIES LABELS client TIMEOUT 75)
add_test(NAME win32_client_host_library_dummy COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tests/compat/run_hostgames.py" "$<TARGET_FILE:stars-compat-client-startup>")
set_tests_properties(win32_client_host_library_dummy PROPERTIES LABELS client TIMEOUT 75)

foreach(STARS_CLIENT_SCENARIO startup race mac_shortcuts)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/${STARS_CLIENT_SCENARIO}")
    add_test(NAME win32_client_${STARS_CLIENT_SCENARIO}_dummy COMMAND stars-compat-client-startup)
    set_tests_properties(win32_client_${STARS_CLIENT_SCENARIO}_dummy PROPERTIES LABELS client TIMEOUT 60
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/${STARS_CLIENT_SCENARIO}"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat;STARS_SETTINGS_DIR=${CMAKE_BINARY_DIR}/test-client/${STARS_CLIENT_SCENARIO}")
endforeach()
set_property(TEST win32_client_mac_shortcuts_dummy APPEND PROPERTY ENVIRONMENT
    "STARS_CLIENT_MAC_SHORTCUTS_TEST=1")
set_property(TEST win32_client_race_dummy APPEND PROPERTY ENVIRONMENT
    "STARS_CLIENT_RACE_CAPTURE=${CMAKE_BINARY_DIR}/test-client/race/habitat.bmp")

add_executable(stars-native-tutorial tests/compat/native_tutorial.cpp)
target_link_libraries(stars-native-tutorial PRIVATE stars_core stars-ui-compat-check
    stars_options stars_win32 SDL3::SDL3)
target_include_directories(stars-native-tutorial PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_definitions(stars-native-tutorial PRIVATE STARS_TEST_TUTORIAL=1)
target_compile_options(stars-native-tutorial PRIVATE -Wall -Wextra -Werror)

set_target_properties(stars-native-tutorial PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME win32_native_tutorial_smoke_dummy COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tests/compat/run_native_tutorial.py" "$<TARGET_FILE:stars-native-tutorial>"
    "${CMAKE_BINARY_DIR}/test-client/tutorial-dummy")
set_tests_properties(win32_native_tutorial_smoke_dummy PROPERTIES LABELS "client;tutorial-smoke" TIMEOUT 60
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat")

add_executable(stars-compat-controls tests/compat/win32_controls.cpp)
target_include_directories(stars-compat-controls PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_link_libraries(stars-compat-controls PRIVATE stars_options stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-controls PRIVATE -Wall -Wextra -Werror)
set_target_properties(stars-compat-controls PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME win32_controls COMMAND stars-compat-controls)
set_tests_properties(win32_controls PROPERTIES LABELS compat TIMEOUT 60
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software")

add_executable(stars-compat-messagebox tests/compat/win32_messagebox.cpp)
target_include_directories(stars-compat-messagebox PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_link_libraries(stars-compat-messagebox PRIVATE stars_options stars_win32 SDL3::SDL3)

target_compile_options(stars-compat-messagebox PRIVATE -Wall -Wextra -Werror)
set_target_properties(stars-compat-messagebox PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME win32_messagebox COMMAND stars-compat-messagebox)
set_tests_properties(win32_messagebox PROPERTIES LABELS compat TIMEOUT 30
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software")

foreach(STARS_UTF8_TEST utf8 font edit profile)
    add_executable(stars-compat-${STARS_UTF8_TEST} tests/compat/win32_${STARS_UTF8_TEST}.cpp)
    target_include_directories(stars-compat-${STARS_UTF8_TEST} PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
    target_link_libraries(stars-compat-${STARS_UTF8_TEST} PRIVATE stars_options stars_win32 SDL3::SDL3)
    if(STARS_UTF8_TEST STREQUAL "utf8" OR STARS_UTF8_TEST STREQUAL "profile")
        target_link_libraries(stars-compat-${STARS_UTF8_TEST} PRIVATE stars_filesystem)
    endif()
    target_compile_options(stars-compat-${STARS_UTF8_TEST} PRIVATE -Wall -Wextra -Werror)
    add_test(NAME win32_utf8_${STARS_UTF8_TEST} COMMAND stars-compat-${STARS_UTF8_TEST})
    set_tests_properties(win32_utf8_${STARS_UTF8_TEST} PROPERTIES LABELS compat TIMEOUT 60
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat")
endforeach()

add_executable(stars-compat-help tests/compat/win32_help.cpp)
target_include_directories(stars-compat-help PRIVATE "${STARS_HELP_GENERATED}")
target_link_libraries(stars-compat-help PRIVATE stars_options stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-help PRIVATE -Wall -Wextra -Werror)

add_dependencies(stars-compat-help stars-help-guide)
add_test(NAME win32_utf8_help COMMAND stars-compat-help "${STARS_HELP_GENERATED}")
set_tests_properties(win32_utf8_help PROPERTIES LABELS compat TIMEOUT 120)

include("${CMAKE_CURRENT_LIST_DIR}/fileworkflow-tests.cmake")

include("${CMAKE_CURRENT_LIST_DIR}/print-map-tests.cmake")
