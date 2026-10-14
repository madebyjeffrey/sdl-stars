# Portable client workflows also qualify the macOS desktop.
add_executable(stars-compat-client-startup tests/compat/win32_client_startup.c)
target_link_libraries(stars-compat-client-startup PRIVATE stars_core stars-ui-compat-check
    stars_options stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-client-startup PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_definitions(stars-compat-client-startup PRIVATE STARS_TEST_CLIENT_STARTUP=1)
target_compile_options(stars-compat-client-startup PRIVATE -Wall -Wextra -Werror)
stars_test_redirect(stars-compat-client-startup InitInstance AlertSz SDL_ShowWindow PromptPassword TrackPopupMenu SDL_RenderPresent SDL_GetPrefPath NewGameWizard FCreateFile)
set_target_properties(stars-compat-client-startup PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME win32_client_continue_dummy COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tests/compat/run_client_continue.py" "$<TARGET_FILE:stars-compat-client-startup>")
set_tests_properties(win32_client_continue_dummy PROPERTIES LABELS client TIMEOUT 200
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat")

add_test(NAME win32_client_newgame_dummy COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tests/compat/run_newgame.py" "$<TARGET_FILE:stars-compat-client-startup>")
set_tests_properties(win32_client_newgame_dummy PROPERTIES LABELS client TIMEOUT 200
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat")

foreach(STARS_CLIENT_SCENARIO startup race)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/${STARS_CLIENT_SCENARIO}")
    add_test(NAME win32_client_${STARS_CLIENT_SCENARIO}_dummy COMMAND stars-compat-client-startup)
    set_tests_properties(win32_client_${STARS_CLIENT_SCENARIO}_dummy PROPERTIES LABELS client TIMEOUT 60
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/${STARS_CLIENT_SCENARIO}"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat;STARS_SETTINGS_DIR=${CMAKE_BINARY_DIR}/test-client/${STARS_CLIENT_SCENARIO}")
endforeach()
set_property(TEST win32_client_race_dummy APPEND PROPERTY ENVIRONMENT
    "STARS_CLIENT_RACE_CAPTURE=${CMAKE_BINARY_DIR}/test-client/race/habitat.bmp")

add_executable(stars-native-tutorial tests/compat/native_tutorial.c)
target_link_libraries(stars-native-tutorial PRIVATE stars_core stars-ui-compat-check
    stars_options stars_win32 SDL3::SDL3)
target_include_directories(stars-native-tutorial PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_definitions(stars-native-tutorial PRIVATE STARS_TEST_TUTORIAL=1)
target_compile_options(stars-native-tutorial PRIVATE -Wall -Wextra -Werror)
stars_test_redirect(stars-native-tutorial InitInstance MessageBoxA TrackPopupMenu)
set_target_properties(stars-native-tutorial PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME win32_native_tutorial_smoke_dummy COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tests/compat/run_native_tutorial.py" "$<TARGET_FILE:stars-native-tutorial>"
    "${CMAKE_BINARY_DIR}/test-client/tutorial-dummy")
set_tests_properties(win32_native_tutorial_smoke_dummy PROPERTIES LABELS "client;tutorial-smoke" TIMEOUT 60
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat")

foreach(STARS_UTF8_TEST utf8 font edit profile)
    add_executable(stars-compat-${STARS_UTF8_TEST} tests/compat/win32_${STARS_UTF8_TEST}.c)
    target_include_directories(stars-compat-${STARS_UTF8_TEST} PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
    target_link_libraries(stars-compat-${STARS_UTF8_TEST} PRIVATE stars_options stars_win32 SDL3::SDL3)
    target_compile_options(stars-compat-${STARS_UTF8_TEST} PRIVATE -Wall -Wextra -Werror)
    add_test(NAME win32_utf8_${STARS_UTF8_TEST} COMMAND stars-compat-${STARS_UTF8_TEST})
    set_tests_properties(win32_utf8_${STARS_UTF8_TEST} PROPERTIES LABELS compat TIMEOUT 60
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat")
endforeach()

add_executable(stars-compat-help tests/compat/win32_help.c)
target_include_directories(stars-compat-help PRIVATE "${STARS_HELP_GENERATED}")
target_link_libraries(stars-compat-help PRIVATE stars_options stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-help PRIVATE -Wall -Wextra -Werror)
stars_test_redirect(stars-compat-help SDL_GetBasePath SDL_GetPrefPath SDL_OpenURL)
add_dependencies(stars-compat-help stars-help-guide)
add_test(NAME win32_utf8_help COMMAND stars-compat-help "${STARS_HELP_GENERATED}")
set_tests_properties(win32_utf8_help PROPERTIES LABELS compat TIMEOUT 120)
