# The Linux desktop harness uses X11, D-Bus and GNU ELF wrapping. Windows
# exercises the same wrapper with portable tests and the shared game entry.
if(STARS_NATIVE_FILE_DIALOGS)
    add_executable(stars-compat-filedialog-windows tests/compat/win32_filedialog_windows.c
        $<TARGET_OBJECTS:stars_filedialog_windows>)
    target_include_directories(stars-compat-filedialog-windows PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
    target_compile_options(stars-compat-filedialog-windows PRIVATE -Wall -Wextra -Wpedantic -Werror)
    set_target_properties(stars-compat-filedialog-windows PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_test(NAME win32_sdl_native_filedialog COMMAND stars-compat-filedialog-windows)
    set_tests_properties(win32_sdl_native_filedialog PROPERTIES LABELS compat TIMEOUT 30)
    # Real desktop checks are opt-in; headless CI never opens system windows.
    add_executable(stars-compat-filedialog-desktop tests/compat/win32_filedialog_windows_desktop.c
        $<TARGET_OBJECTS:stars_filedialog_windows>)
    target_link_libraries(stars-compat-filedialog-desktop PRIVATE comdlg32)
    target_include_directories(stars-compat-filedialog-desktop PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
    target_compile_options(stars-compat-filedialog-desktop PRIVATE -Wall -Wextra -Wpedantic -Werror)
    set_target_properties(stars-compat-filedialog-desktop PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_executable(stars-compat-filedialog-native-flow tests/compat/win32_filedialog_windows_flow.c)
    target_link_libraries(stars-compat-filedialog-native-flow PRIVATE stars_win32 SDL3::SDL3)
    target_include_directories(stars-compat-filedialog-native-flow PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
    target_link_options(stars-compat-filedialog-native-flow PRIVATE "LINKER:--wrap=IChooseWindowsFile")
    target_compile_options(stars-compat-filedialog-native-flow PRIVATE -Wall -Wextra -Wpedantic -Werror)
    set_target_properties(stars-compat-filedialog-native-flow PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-native-filedialog")
    add_test(NAME win32_sdl_native_fileflow COMMAND stars-compat-filedialog-native-flow)
    set_tests_properties(win32_sdl_native_fileflow PROPERTIES LABELS compat TIMEOUT 30
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test-native-filedialog"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software")
endif()
add_test(NAME win32_sdl_abi COMMAND stars-compat-abi)
add_test(NAME sdl_smoke_software COMMAND stars-compat-smoke --self-test)
set_tests_properties(sdl_smoke_software PROPERTIES LABELS compat TIMEOUT 30
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software")

foreach(STARS_COMPAT_TEST windows bitmap dib memory timers controls edit regions geometry children position capture input menu popup combo palette)
    add_executable(stars-compat-${STARS_COMPAT_TEST} tests/compat/win32_${STARS_COMPAT_TEST}.c)
    target_link_libraries(stars-compat-${STARS_COMPAT_TEST} PRIVATE stars_win32 SDL3::SDL3)
    target_include_directories(stars-compat-${STARS_COMPAT_TEST} PRIVATE
        "${PROJECT_SOURCE_DIR}/compat/win32/src" "${STARS_BITMAP_GENERATED}")
    add_dependencies(stars-compat-${STARS_COMPAT_TEST} stars_bitmap_resources)
    target_compile_options(stars-compat-${STARS_COMPAT_TEST} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    set_target_properties(stars-compat-${STARS_COMPAT_TEST} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_test(NAME win32_sdl_${STARS_COMPAT_TEST} COMMAND stars-compat-${STARS_COMPAT_TEST} --self-test)
    set_tests_properties(win32_sdl_${STARS_COMPAT_TEST} PROPERTIES LABELS compat TIMEOUT 30
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software")
endforeach()
target_sources(stars-compat-dib PRIVATE tests/compat/win32_dib_invalid.c)
target_link_options(stars-compat-input PRIVATE
    "LINKER:--wrap=SDL_GetKeyboardState,--wrap=SDL_GetModState,--wrap=SDL_GetGlobalMouseState")

add_executable(stars-compat-windows-platform tests/compat/win32_windows_platform.c)
add_library(stars-compat-platform-fixture OBJECT tests/compat/win32_platform_fixture.c)
target_sources(stars-compat-windows-platform PRIVATE $<TARGET_OBJECTS:stars-compat-platform-fixture>)
target_link_libraries(stars-compat-windows-platform PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-windows-platform PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-windows-platform PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-windows-platform PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-windows-platform")
add_test(NAME win32_sdl_platform COMMAND stars-compat-windows-platform)
set_tests_properties(win32_sdl_platform PROPERTIES LABELS compat TIMEOUT 30
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test-windows-platform")

add_executable(stars-compat-client-startup tests/compat/win32_client_startup.c)
target_link_libraries(stars-compat-client-startup PRIVATE stars_core stars-ui-compat-check
    stars_options stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-client-startup PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_definitions(stars-compat-client-startup PRIVATE STARS_TEST_CLIENT_STARTUP=1
    __wrap_TrackPopupMenu=__wrap_StarsCompat_TrackPopupMenu
    __real_TrackPopupMenu=__real_StarsCompat_TrackPopupMenu)
target_compile_options(stars-compat-client-startup PRIVATE -Wall -Wextra -Werror)
target_link_options(stars-compat-client-startup PRIVATE
    "LINKER:--wrap=InitInstance,--wrap=AlertSz,--wrap=SDL_ShowWindow,--wrap=PromptPassword,--wrap=StarsCompat_TrackPopupMenu,--wrap=SDL_RenderPresent,--wrap=NewGameWizard,--wrap=FNativeSaveFileName")
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
target_compile_definitions(stars-native-tutorial PRIVATE STARS_TEST_TUTORIAL=1
    __wrap_TrackPopupMenu=__wrap_StarsCompat_TrackPopupMenu
    __real_TrackPopupMenu=__real_StarsCompat_TrackPopupMenu
    __wrap_MessageBoxA=__wrap_StarsCompat_MessageBoxA
    __real_MessageBoxA=__real_StarsCompat_MessageBoxA)
target_compile_options(stars-native-tutorial PRIVATE -Wall -Wextra -Werror)
target_link_options(stars-native-tutorial PRIVATE
    "LINKER:--wrap=InitInstance,--wrap=StarsCompat_MessageBoxA,--wrap=StarsCompat_TrackPopupMenu")
set_target_properties(stars-native-tutorial PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME win32_native_tutorial_smoke_dummy COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tests/compat/run_native_tutorial.py" "$<TARGET_FILE:stars-native-tutorial>"
    "${CMAKE_BINARY_DIR}/test-client/tutorial-dummy")
set_tests_properties(win32_native_tutorial_smoke_dummy PROPERTIES LABELS "client;tutorial-smoke" TIMEOUT 60
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat")
