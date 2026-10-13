include("${CMAKE_CURRENT_LIST_DIR}/dependencies/sdl3.cmake")
if(APPLE)
    # Chained fixups require aligned pointers; the original packed UI tables do not.
    target_link_options(stars_options INTERFACE "LINKER:-no_fixup_chains")
endif()
add_subdirectory(compat/win32)
find_program(STARS_WMF2SVG_EXECUTABLE NAMES wmf2svg REQUIRED)
set(STARS_HELP_GENERATED "${CMAKE_BINARY_DIR}/help")
add_custom_command(OUTPUT "${STARS_HELP_GENERATED}/complete.stamp" "${STARS_HELP_GENERATED}/help_contexts.h"
    BYPRODUCTS "${CMAKE_BINARY_DIR}/bin/stars!.hlp"
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/compat/win32/hlp_render.py"
        "${PROJECT_SOURCE_DIR}/res/stars!.hlp" "${STARS_HELP_GENERATED}"
        --wmf2svg "${STARS_WMF2SVG_EXECUTABLE}"
        --header "${STARS_HELP_GENERATED}/help_contexts.h"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/bin"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${PROJECT_SOURCE_DIR}/res/stars!.hlp" "${CMAKE_BINARY_DIR}/bin/stars!.hlp"
    DEPENDS res/stars!.hlp compat/win32/hlp.py compat/win32/hlp_format.py
        compat/win32/hlp_font.py compat/win32/hlp_image.py compat/win32/hlp_render.py
        compat/win32/help.css compat/win32/help.js compat/win32/help_bundle.js res/fonts/LICENSE
        res/fonts/LiberationSans-Regular.ttf res/fonts/LiberationSans-Bold.ttf
        res/fonts/LiberationSans-Italic.ttf res/fonts/LiberationSans-BoldItalic.ttf
    COMMENT "Generating the original Stars! help guide" VERBATIM)
add_custom_target(stars-help-guide ALL DEPENDS "${STARS_HELP_GENERATED}/complete.stamp")
add_dependencies(stars_win32 stars-help-guide)
target_include_directories(stars_win32 PRIVATE "${STARS_HELP_GENERATED}")
if(STARS_BUILD_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
add_executable(stars-compat-help tests/compat/win32_help.c)
target_link_libraries(stars-compat-help PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-help PRIVATE "${STARS_HELP_GENERATED}")
target_compile_options(stars-compat-help PRIVATE -Wall -Wextra -Wpedantic -Werror)
stars_test_redirect(stars-compat-help SDL_GetBasePath SDL_OpenURL)
add_dependencies(stars-compat-help stars-help-guide)
set_target_properties(stars-compat-help PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
add_executable(stars-compat-help-viewer tests/compat/win32_help_viewer.c)
target_link_libraries(stars-compat-help-viewer PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-help-viewer PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_dependencies(stars-compat-help-viewer stars-help-guide)
set_target_properties(stars-compat-help-viewer PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_test(NAME win32_help COMMAND stars-compat-help "${STARS_HELP_GENERATED}" "${PROJECT_SOURCE_DIR}/res/stars!.hlp")
    set_tests_properties(win32_help PROPERTIES LABELS "compat" TIMEOUT 90)
    add_test(NAME win32_help_archive COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/compat/test_hlp.py")
    set_tests_properties(win32_help_archive PROPERTIES LABELS "compat" TIMEOUT 30)
endif()

# The client and compile check share the existing Windows UI sources.
add_library(stars-ui-compat-check OBJECT EXCLUDE_FROM_ALL ${STARS_UI_SOURCES}
    "${STARS_GENERATED_DIR}/text_tutorial.c")
target_link_libraries(stars-ui-compat-check PRIVATE stars_options stars_win32_headers)
target_compile_options(stars-ui-compat-check PRIVATE
    -Werror=implicit-function-declaration -Werror=incompatible-pointer-types)
add_dependencies(stars-ui-compat-check stars_version)
add_executable(stars-sdl compat/win32/src/entry.c)
option(STARS_TEST_TRACE "Log selected game calls for regression diagnostics" OFF)
if(STARS_TEST_TRACE)
    add_library(stars-regression-trace OBJECT tests/scaffold/regression_trace.c)
    target_link_libraries(stars-regression-trace PRIVATE stars_options stars_win32_headers)
    target_link_libraries(stars-sdl PRIVATE stars-regression-trace)
endif()
target_link_libraries(stars-sdl PRIVATE stars_core stars-ui-compat-check stars_options stars_win32 SDL3::SDL3)
if(STARS_TEST_TRACE)
    stars_test_redirect(stars-sdl Random PctPlanetCapacity)
endif()
target_compile_options(stars-sdl PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-sdl PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
include("${CMAKE_CURRENT_LIST_DIR}/install.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/sdl-unit-tests.cmake")
if(NOT STARS_BUILD_TESTS)
    return()
endif()
# The qualification harness below uses GNU ld wrapping, X11 and private DBus.
# Build the product and help viewer on macOS without Linux-only harness targets.
add_executable(stars-compat-client-quit tests/compat/win32_client_quit.c)
if(APPLE)
    enable_language(OBJC)
    target_sources(stars-compat-client-quit PRIVATE tests/compat/win32_client_quit_cocoa.m)
    set_source_files_properties(tests/compat/win32_client_quit_cocoa.m PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
endif()
target_link_libraries(stars-compat-client-quit PRIVATE stars_core stars-ui-compat-check stars_options stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-client-quit PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-client-quit PRIVATE -Wall -Wextra -Werror)
foreach(STARS_QUIT_PAGE simple advanced race save-owner advanced-save-owner)
    add_test(NAME win32_client_quit_${STARS_QUIT_PAGE} COMMAND stars-compat-client-quit ${STARS_QUIT_PAGE})
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/quit-${STARS_QUIT_PAGE}")
    set_tests_properties(win32_client_quit_${STARS_QUIT_PAGE} PROPERTIES LABELS "client"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/quit-${STARS_QUIT_PAGE}"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat"
        TIMEOUT 20)
endforeach()
if(APPLE OR WIN32)
    if(APPLE)
        add_executable(stars-compat-filedialog-cocoa tests/compat/win32_filedialog_cocoa.c
            compat/win32/src/filedialog_cocoa.c)
        target_include_directories(stars-compat-filedialog-cocoa PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
        target_link_libraries(stars-compat-filedialog-cocoa PRIVATE stars_options stars_win32 SDL3::SDL3)
        target_compile_options(stars-compat-filedialog-cocoa PRIVATE -Wall -Wextra -Wpedantic -Werror)
        add_test(NAME win32_filedialog_cocoa COMMAND stars-compat-filedialog-cocoa)
        set_tests_properties(win32_filedialog_cocoa PROPERTIES LABELS "compat" TIMEOUT 30)
    endif()
    add_executable(stars-compat-abi tests/compat/win32_abi.c tests/compat/win32_include_order.c)
    target_link_libraries(stars-compat-abi PRIVATE stars_options stars_win32_headers)
    add_executable(stars-compat-smoke tests/compat/sdl_smoke.c)
    target_link_libraries(stars-compat-smoke PRIVATE SDL3::SDL3)
    set_target_properties(stars-compat-abi stars-compat-smoke PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    if(APPLE)
        add_executable(stars-compat-runtime tests/compat/win32_runtime.c tests/compat/win32_tick.c)
        target_link_libraries(stars-compat-runtime PRIVATE stars_win32 SDL3::SDL3)
        target_compile_options(stars-compat-runtime PRIVATE -Wall -Wextra -Wpedantic -Werror)
        stars_test_redirect(stars-compat-runtime SDL_GetTicks)
        set_target_properties(stars-compat-runtime PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
        add_test(NAME win32_runtime COMMAND stars-compat-runtime)
        set_tests_properties(win32_runtime PROPERTIES LABELS "compat" TIMEOUT 30)
    endif()
    if(WIN32)
        include("${CMAKE_CURRENT_LIST_DIR}/windows-sdl-tests.cmake")
    endif()
    return()
endif()
add_executable(stars-compat-entry compat/win32/src/entry.c tests/compat/win32_entry.c)
target_link_libraries(stars-compat-entry PRIVATE stars_options stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-entry PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_executable(stars-compat-session tests/compat/win32_session.c)
target_link_libraries(stars-compat-session PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-session PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_executable(stars-compat-process-exit tests/compat/win32_process_exit.c)
target_link_libraries(stars-compat-process-exit PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-process-exit PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_executable(stars-compat-client-startup tests/compat/win32_client_startup.c)
target_link_libraries(stars-compat-client-startup PRIVATE stars_core stars-ui-compat-check stars_options stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-client-startup PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_definitions(stars-compat-client-startup PRIVATE STARS_TEST_CLIENT_STARTUP=1)
target_compile_options(stars-compat-client-startup PRIVATE -Wall -Wextra -Werror)
stars_test_redirect(stars-compat-client-startup InitInstance AlertSz SDL_ShowWindow PromptPassword TrackPopupMenu SDL_RenderPresent NewGameWizard FNativeSaveFileName)
add_executable(stars-native-tutorial tests/compat/native_tutorial.c)
add_executable(stars-compat-fileworkflow tests/compat/win32_fileworkflow.c)
target_link_libraries(stars-compat-fileworkflow PRIVATE stars_core stars-ui-compat-check stars_options stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-fileworkflow PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_definitions(stars-compat-fileworkflow PRIVATE STARS_TEST_FILE_WORKFLOW=1)
target_compile_options(stars-compat-fileworkflow PRIVATE -Wall -Wextra -Werror)
stars_test_redirect(stars-compat-fileworkflow InitInstance AlertSz PromptPassword)
set_target_properties(stars-compat-fileworkflow PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
target_link_libraries(stars-native-tutorial PRIVATE stars_core stars-ui-compat-check stars_options stars_win32 SDL3::SDL3)
target_include_directories(stars-native-tutorial PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_definitions(stars-native-tutorial PRIVATE STARS_TEST_TUTORIAL=1)
target_compile_options(stars-native-tutorial PRIVATE -Wall -Wextra -Werror)
stars_test_redirect(stars-native-tutorial InitInstance MessageBoxA TrackPopupMenu)
foreach(STARS_ENTRY_TARGET stars-compat-entry stars-compat-session stars-compat-process-exit stars-compat-client-startup stars-native-tutorial)
    set_target_properties(${STARS_ENTRY_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endforeach()
option(STARS_TUTORIAL_CHECKPOINT_TESTS "Replay every earlier bounded tutorial stop as a separate test" OFF)
if(STARS_BUILD_TESTS)
    add_test(NAME win32_entry COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/compat/test_entry.py" "$<TARGET_FILE:stars-compat-entry>")
    add_test(NAME win32_session COMMAND stars-compat-session)
    add_test(NAME win32_process_exit COMMAND stars-compat-process-exit)
    set_tests_properties(win32_process_exit PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
    set_tests_properties(win32_entry win32_session PROPERTIES LABELS "compat" TIMEOUT 30)
    add_test(NAME win32_client_continue_dummy COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_client_continue.py" "$<TARGET_FILE:stars-compat-client-startup>")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/continue-dummy")
    set_tests_properties(win32_client_continue_dummy PROPERTIES LABELS "client"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_CLIENT_GAME_CAPTURE=${CMAKE_BINARY_DIR}/test-client/continue-dummy/game.bmp" TIMEOUT 200)
    find_program(STARS_XVFB_RUN_EXECUTABLE NAMES xvfb-run REQUIRED)
    find_program(STARS_OPENBOX_EXECUTABLE NAMES openbox REQUIRED)
    find_program(STARS_XPROP_EXECUTABLE NAMES xprop REQUIRED)
    foreach(STARS_TUTORIAL_BACKEND dummy x11)
        set(STARS_TUTORIAL_COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/compat/run_native_tutorial.py"
            "$<TARGET_FILE:stars-native-tutorial>" "${CMAKE_BINARY_DIR}/test-client/tutorial-${STARS_TUTORIAL_BACKEND}")
        if(STARS_TUTORIAL_BACKEND STREQUAL "x11")
            add_test(NAME win32_native_tutorial_smoke_x11 COMMAND "${Python3_EXECUTABLE}"
                "${PROJECT_SOURCE_DIR}/tests/compat/run_x11_wm.py" --xvfb "${STARS_XVFB_RUN_EXECUTABLE}"
                --wm "${STARS_OPENBOX_EXECUTABLE}" --xprop "${STARS_XPROP_EXECUTABLE}" --timeout 40 ${STARS_TUTORIAL_COMMAND})
            set(STARS_TUTORIAL_DRIVER x11)
        else()
            add_test(NAME win32_native_tutorial_smoke_dummy COMMAND ${STARS_TUTORIAL_COMMAND})
            set(STARS_TUTORIAL_DRIVER dummy)
        endif()
        set_tests_properties(win32_native_tutorial_smoke_${STARS_TUTORIAL_BACKEND} PROPERTIES LABELS "client;tutorial-smoke"
            ENVIRONMENT "SDL_VIDEODRIVER=${STARS_TUTORIAL_DRIVER};SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 45 RUN_SERIAL TRUE)
        if(STARS_TUTORIAL_CHECKPOINT_TESTS)
            set(STARS_TUTORIAL_YEAR_COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/compat/run_native_tutorial.py"
                "$<TARGET_FILE:stars-native-tutorial>" "${CMAKE_BINARY_DIR}/test-client/tutorial-first-year-${STARS_TUTORIAL_BACKEND}" first-year)
            if(STARS_TUTORIAL_BACKEND STREQUAL "x11")
                add_test(NAME win32_native_tutorial_first_year_x11 COMMAND "${Python3_EXECUTABLE}"
                    "${PROJECT_SOURCE_DIR}/tests/compat/run_x11_wm.py" --xvfb "${STARS_XVFB_RUN_EXECUTABLE}"
                    --wm "${STARS_OPENBOX_EXECUTABLE}" --xprop "${STARS_XPROP_EXECUTABLE}" --timeout 60 ${STARS_TUTORIAL_YEAR_COMMAND})
            else()
                add_test(NAME win32_native_tutorial_first_year_dummy COMMAND ${STARS_TUTORIAL_YEAR_COMMAND})
            endif()
            set_tests_properties(win32_native_tutorial_first_year_${STARS_TUTORIAL_BACKEND} PROPERTIES LABELS "client;tutorial-partial"
                ENVIRONMENT "SDL_VIDEODRIVER=${STARS_TUTORIAL_DRIVER};SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 65 RUN_SERIAL TRUE)
            set(STARS_TUTORIAL_TWO_COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/compat/run_native_tutorial.py"
                "$<TARGET_FILE:stars-native-tutorial>" "${CMAKE_BINARY_DIR}/test-client/tutorial-two-years-${STARS_TUTORIAL_BACKEND}" two-years)
            if(STARS_TUTORIAL_BACKEND STREQUAL "x11")
                add_test(NAME win32_native_tutorial_two_years_x11 COMMAND "${Python3_EXECUTABLE}"
                    "${PROJECT_SOURCE_DIR}/tests/compat/run_x11_wm.py" --xvfb "${STARS_XVFB_RUN_EXECUTABLE}"
                    --wm "${STARS_OPENBOX_EXECUTABLE}" --xprop "${STARS_XPROP_EXECUTABLE}" --timeout 90 ${STARS_TUTORIAL_TWO_COMMAND})
            else()
                add_test(NAME win32_native_tutorial_two_years_dummy COMMAND ${STARS_TUTORIAL_TWO_COMMAND})
            endif()
            set_tests_properties(win32_native_tutorial_two_years_${STARS_TUTORIAL_BACKEND} PROPERTIES LABELS "client;tutorial-partial"
                ENVIRONMENT "SDL_VIDEODRIVER=${STARS_TUTORIAL_DRIVER};SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 100 RUN_SERIAL TRUE)
        endif()
        set(STARS_TUTORIAL_SCENARIOS merge-cycle)
        if(STARS_TUTORIAL_CHECKPOINT_TESTS)
            list(PREPEND STARS_TUTORIAL_SCENARIOS scouting mining colonization logistics colonizer-production second-colonization transport-cycle expansion fleet-split research-cycle oxygen-colonization production-templates battle-cycle terraform-cycle design-cycle automation-cycle report-cycle)
        endif()
        foreach(STARS_TUTORIAL_SCENARIO IN LISTS STARS_TUTORIAL_SCENARIOS)
            if(STARS_TUTORIAL_SCENARIO STREQUAL "merge-cycle")
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 1260)
                set(STARS_TUTORIAL_TEST_TIMEOUT 1270)
            elseif(STARS_TUTORIAL_SCENARIO STREQUAL "report-cycle")
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 1140)
                set(STARS_TUTORIAL_TEST_TIMEOUT 1150)
            elseif(STARS_TUTORIAL_SCENARIO STREQUAL "automation-cycle")
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 1020)
                set(STARS_TUTORIAL_TEST_TIMEOUT 1030)
            elseif(STARS_TUTORIAL_SCENARIO STREQUAL "design-cycle")
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 960)
                set(STARS_TUTORIAL_TEST_TIMEOUT 970)
            elseif(STARS_TUTORIAL_SCENARIO STREQUAL "terraform-cycle")
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 840)
                set(STARS_TUTORIAL_TEST_TIMEOUT 850)
            elseif(STARS_TUTORIAL_SCENARIO STREQUAL "battle-cycle")
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 780)
                set(STARS_TUTORIAL_TEST_TIMEOUT 790)
            elseif(STARS_TUTORIAL_SCENARIO STREQUAL "production-templates")
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 660)
                set(STARS_TUTORIAL_TEST_TIMEOUT 670)
            elseif(STARS_TUTORIAL_SCENARIO STREQUAL "oxygen-colonization")
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 600)
                set(STARS_TUTORIAL_TEST_TIMEOUT 610)
            elseif(STARS_TUTORIAL_SCENARIO STREQUAL "research-cycle")
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 540)
                set(STARS_TUTORIAL_TEST_TIMEOUT 550)
            elseif(STARS_TUTORIAL_SCENARIO STREQUAL "fleet-split")
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 480)
                set(STARS_TUTORIAL_TEST_TIMEOUT 490)
            elseif(STARS_TUTORIAL_SCENARIO STREQUAL "expansion")
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 450)
                set(STARS_TUTORIAL_TEST_TIMEOUT 460)
            elseif(STARS_TUTORIAL_SCENARIO STREQUAL "transport-cycle")
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 390)
                set(STARS_TUTORIAL_TEST_TIMEOUT 400)
            elseif(STARS_TUTORIAL_SCENARIO STREQUAL "second-colonization")
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 330)
                set(STARS_TUTORIAL_TEST_TIMEOUT 340)
            elseif(STARS_TUTORIAL_SCENARIO STREQUAL "colonizer-production")
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 300)
                set(STARS_TUTORIAL_TEST_TIMEOUT 310)
            elseif(STARS_TUTORIAL_SCENARIO STREQUAL "logistics")
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 270)
                set(STARS_TUTORIAL_TEST_TIMEOUT 280)
            elseif(STARS_TUTORIAL_SCENARIO STREQUAL "colonization")
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 210)
                set(STARS_TUTORIAL_TEST_TIMEOUT 220)
            else()
                set(STARS_TUTORIAL_WRAPPER_TIMEOUT 150)
                set(STARS_TUTORIAL_TEST_TIMEOUT 160)
            endif()
            set(STARS_TUTORIAL_EXTENDED_COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/compat/run_native_tutorial.py"
                "$<TARGET_FILE:stars-native-tutorial>" "${CMAKE_BINARY_DIR}/test-client/tutorial-${STARS_TUTORIAL_SCENARIO}-${STARS_TUTORIAL_BACKEND}" ${STARS_TUTORIAL_SCENARIO})
            if(STARS_TUTORIAL_BACKEND STREQUAL "x11")
                add_test(NAME win32_native_tutorial_${STARS_TUTORIAL_SCENARIO}_x11 COMMAND "${Python3_EXECUTABLE}"
                    "${PROJECT_SOURCE_DIR}/tests/compat/run_x11_wm.py" --xvfb "${STARS_XVFB_RUN_EXECUTABLE}"
                    --wm "${STARS_OPENBOX_EXECUTABLE}" --xprop "${STARS_XPROP_EXECUTABLE}" --timeout ${STARS_TUTORIAL_WRAPPER_TIMEOUT} ${STARS_TUTORIAL_EXTENDED_COMMAND})
            else()
                add_test(NAME win32_native_tutorial_${STARS_TUTORIAL_SCENARIO}_dummy COMMAND ${STARS_TUTORIAL_EXTENDED_COMMAND})
            endif()
            set_tests_properties(win32_native_tutorial_${STARS_TUTORIAL_SCENARIO}_${STARS_TUTORIAL_BACKEND} PROPERTIES LABELS "client;tutorial-partial"
                ENVIRONMENT "SDL_VIDEODRIVER=${STARS_TUTORIAL_DRIVER};SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT ${STARS_TUTORIAL_TEST_TIMEOUT} RUN_SERIAL TRUE)
        endforeach()
    endforeach()
    add_test(NAME win32_client_continue_x11 COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_x11_wm.py"
        --xvfb "${STARS_XVFB_RUN_EXECUTABLE}" --wm "${STARS_OPENBOX_EXECUTABLE}"
        --xprop "${STARS_XPROP_EXECUTABLE}" --timeout 200 "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_client_continue.py" "$<TARGET_FILE:stars-compat-client-startup>")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/continue-x11")
    set_tests_properties(win32_client_continue_x11 PROPERTIES LABELS "client"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/continue-x11"
        ENVIRONMENT "SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 210)
    add_test(NAME win32_client_fleet_dummy COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_client_continue.py" "$<TARGET_FILE:stars-compat-client-startup>")
    set_tests_properties(win32_client_fleet_dummy PROPERTIES LABELS "client"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_CLIENT_WORKFLOW=fleet" TIMEOUT 200)
    add_test(NAME win32_client_fleet_x11 COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_x11_wm.py"
        --xvfb "${STARS_XVFB_RUN_EXECUTABLE}" --wm "${STARS_OPENBOX_EXECUTABLE}"
        --xprop "${STARS_XPROP_EXECUTABLE}" --timeout 200 "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_client_continue.py" "$<TARGET_FILE:stars-compat-client-startup>")
    set_tests_properties(win32_client_fleet_x11 PROPERTIES LABELS "client"
        ENVIRONMENT "SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_CLIENT_WORKFLOW=fleet" TIMEOUT 210)
    add_test(NAME win32_client_designer_dummy COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_client_continue.py" "$<TARGET_FILE:stars-compat-client-startup>")
    set_tests_properties(win32_client_designer_dummy PROPERTIES LABELS "client"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_CLIENT_WORKFLOW=designer" TIMEOUT 200)
    add_test(NAME win32_client_designer_x11 COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_x11_wm.py"
        --xvfb "${STARS_XVFB_RUN_EXECUTABLE}" --wm "${STARS_OPENBOX_EXECUTABLE}"
        --xprop "${STARS_XPROP_EXECUTABLE}" --timeout 200 "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_client_continue.py" "$<TARGET_FILE:stars-compat-client-startup>")
    set_tests_properties(win32_client_designer_x11 PROPERTIES LABELS "client"
        ENVIRONMENT "SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_CLIENT_WORKFLOW=designer" TIMEOUT 210)
    add_test(NAME win32_client_newgame_dummy COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_newgame.py" "$<TARGET_FILE:stars-compat-client-startup>")
    set_tests_properties(win32_client_newgame_dummy PROPERTIES LABELS "client"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 100)
    add_test(NAME win32_client_newgame_x11 COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_x11_wm.py"
        --xvfb "${STARS_XVFB_RUN_EXECUTABLE}" --wm "${STARS_OPENBOX_EXECUTABLE}"
        --xprop "${STARS_XPROP_EXECUTABLE}" --timeout 100 "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_newgame.py" "$<TARGET_FILE:stars-compat-client-startup>")
    set_tests_properties(win32_client_newgame_x11 PROPERTIES LABELS "client"
        ENVIRONMENT "SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 110)
    add_test(NAME win32_client_polish_dummy COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_client_continue.py" "$<TARGET_FILE:stars-compat-client-startup>")
    set_tests_properties(win32_client_polish_dummy PROPERTIES LABELS "client"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_CLIENT_WORKFLOW=polish;STARS_CLIENT_CHECKPOINT=010" TIMEOUT 200)
    add_test(NAME win32_client_polish_x11 COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_x11_wm.py"
        --xvfb "${STARS_XVFB_RUN_EXECUTABLE}" --wm "${STARS_OPENBOX_EXECUTABLE}"
        --xprop "${STARS_XPROP_EXECUTABLE}" --timeout 200 "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_client_continue.py" "$<TARGET_FILE:stars-compat-client-startup>")
    set_tests_properties(win32_client_polish_x11 PROPERTIES LABELS "client"
        ENVIRONMENT "SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_CLIENT_WORKFLOW=polish;STARS_CLIENT_CHECKPOINT=010" TIMEOUT 210)
    foreach(STARS_STARTUP_BACKEND dummy x11)
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/${STARS_STARTUP_BACKEND}")
        if(STARS_STARTUP_BACKEND STREQUAL "x11")
            add_test(NAME win32_client_startup_x11 COMMAND "${Python3_EXECUTABLE}"
                "${PROJECT_SOURCE_DIR}/tests/compat/run_x11_wm.py"
                --xvfb "${STARS_XVFB_RUN_EXECUTABLE}" --wm "${STARS_OPENBOX_EXECUTABLE}"
                --xprop "${STARS_XPROP_EXECUTABLE}" "$<TARGET_FILE:stars-compat-client-startup>")
        else()
            add_test(NAME win32_client_startup_dummy COMMAND stars-compat-client-startup)
        endif()
        set_tests_properties(win32_client_startup_${STARS_STARTUP_BACKEND} PROPERTIES LABELS "compat"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/${STARS_STARTUP_BACKEND}"
            ENVIRONMENT "SDL_VIDEODRIVER=${STARS_STARTUP_BACKEND};SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_SETTINGS_DIR=${CMAKE_BINARY_DIR}/test-client/${STARS_STARTUP_BACKEND};STARS_CLIENT_CAPTURE=${CMAKE_BINARY_DIR}/test-client/${STARS_STARTUP_BACKEND}/title.bmp;STARS_CLIENT_WIZARD_CAPTURE=${CMAKE_BINARY_DIR}/test-client/${STARS_STARTUP_BACKEND}/wizard.bmp"
            TIMEOUT 30)
        if(STARS_STARTUP_BACKEND STREQUAL "x11")
            add_test(NAME win32_client_race_x11 COMMAND "${Python3_EXECUTABLE}"
                "${PROJECT_SOURCE_DIR}/tests/compat/run_x11_wm.py"
                --xvfb "${STARS_XVFB_RUN_EXECUTABLE}" --wm "${STARS_OPENBOX_EXECUTABLE}"
                --xprop "${STARS_XPROP_EXECUTABLE}" "$<TARGET_FILE:stars-compat-client-startup>")
        else()
            add_test(NAME win32_client_race_dummy COMMAND stars-compat-client-startup)
        endif()
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/race-${STARS_STARTUP_BACKEND}")
        set_tests_properties(win32_client_race_${STARS_STARTUP_BACKEND} PROPERTIES LABELS "client"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/race-${STARS_STARTUP_BACKEND}"
            ENVIRONMENT "SDL_VIDEODRIVER=${STARS_STARTUP_BACKEND};SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_SETTINGS_DIR=${CMAKE_BINARY_DIR}/test-client/race-${STARS_STARTUP_BACKEND};STARS_CLIENT_RACE_CAPTURE=${CMAKE_BINARY_DIR}/test-client/race-${STARS_STARTUP_BACKEND}/habitat.bmp"
            TIMEOUT 30)
    endforeach()
endif()


add_executable(stars-compat-abi tests/compat/win32_abi.c tests/compat/win32_include_order.c)
target_link_libraries(stars-compat-abi PRIVATE stars_options stars_win32_headers)
target_compile_options(stars-compat-abi PRIVATE -Wall -Wextra -Werror)
set_target_properties(stars-compat-abi PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(stars-compat-runtime tests/compat/win32_runtime.c tests/compat/win32_tick.c)
target_link_libraries(stars-compat-runtime PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-runtime PRIVATE -Wall -Wextra -Wpedantic -Werror)
stars_test_redirect(stars-compat-runtime SDL_GetTicks)
set_target_properties(stars-compat-runtime PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(stars-compat-windows tests/compat/win32_windows.c)
target_link_libraries(stars-compat-windows PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-windows PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-windows PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(stars-compat-paint tests/compat/win32_paint.c)
target_link_libraries(stars-compat-paint PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-paint PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-paint PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(stars-compat-gdi-reference tests/compat/win32_gdi_reference.c)
target_link_libraries(stars-compat-gdi-reference PRIVATE stars_win32)
target_compile_options(stars-compat-gdi-reference PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-gdi-reference PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(stars-compat-bitmap tests/compat/win32_bitmap.c)
target_link_libraries(stars-compat-bitmap PRIVATE stars_win32)
target_compile_options(stars-compat-bitmap PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-bitmap PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(stars-compat-dib tests/compat/win32_dib.c tests/compat/win32_dib_invalid.c)
target_link_libraries(stars-compat-dib PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-dib PRIVATE "${STARS_BITMAP_GENERATED}" "${CMAKE_SOURCE_DIR}/compat/win32/src")
add_dependencies(stars-compat-dib stars_bitmap_resources)
target_compile_options(stars-compat-dib PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-dib PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(stars-compat-memory tests/compat/win32_memory.c)
target_link_libraries(stars-compat-memory PRIVATE stars_win32)
target_compile_options(stars-compat-memory PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-memory PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(stars-compat-font tests/compat/win32_font.c)
target_link_libraries(stars-compat-font PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-font PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-font PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(stars-compat-file tests/compat/win32_file.c)
target_link_libraries(stars-compat-file PRIVATE stars_win32)
target_compile_options(stars-compat-file PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-file PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(stars-compat-profile tests/compat/win32_profile.c)
target_link_libraries(stars-compat-profile PRIVATE stars_win32)
target_compile_options(stars-compat-profile PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-profile PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(stars-compat-services tests/compat/win32_services.c)
target_link_libraries(stars-compat-services PRIVATE stars_win32)
target_compile_options(stars-compat-services PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-services PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(stars-compat-timers tests/compat/win32_timers.c)
target_link_libraries(stars-compat-timers PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-timers PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-timers PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# This executable exercises SDL directly, independently of the Win32 subset.
add_executable(stars-compat-smoke tests/compat/sdl_smoke.c)
target_link_libraries(stars-compat-smoke PRIVATE SDL3::SDL3 stars_sanitizers)
target_compile_options(stars-compat-smoke PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(stars-compat-smoke PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(STARS_BUILD_TESTS)
    add_test(NAME win32_abi COMMAND stars-compat-abi)
    set_tests_properties(win32_abi PROPERTIES LABELS "compat")
    add_test(NAME win32_runtime COMMAND stars-compat-runtime)
    set_tests_properties(win32_runtime PROPERTIES LABELS "compat")
    add_test(NAME win32_windows COMMAND stars-compat-windows --self-test)
    set_tests_properties(win32_windows PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy"
        LABELS "sdl;compat"
        TIMEOUT 30)
    add_test(NAME win32_paint COMMAND stars-compat-paint --self-test)
    set_tests_properties(win32_paint PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software"
        LABELS "sdl;compat"
        TIMEOUT 30)
    add_test(NAME win32_gdi_reference COMMAND stars-compat-gdi-reference)
    set_tests_properties(win32_gdi_reference PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software"
        LABELS "sdl;compat"
        TIMEOUT 30)
    add_test(NAME win32_bitmap COMMAND stars-compat-bitmap)
    set_tests_properties(win32_bitmap PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software"
        LABELS "sdl;compat"
        TIMEOUT 30)
    add_test(NAME win32_timers COMMAND stars-compat-timers)
    set_tests_properties(win32_timers PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy" LABELS "compat" TIMEOUT 30)
    add_test(NAME win32_font COMMAND stars-compat-font)
    set_tests_properties(win32_font PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software")
    add_test(NAME win32_file COMMAND stars-compat-file)
    set_tests_properties(win32_file PROPERTIES LABELS "compat")
    add_test(NAME win32_profile COMMAND stars-compat-profile)
    set_tests_properties(win32_profile PROPERTIES LABELS "compat")
    add_test(NAME win32_services COMMAND stars-compat-services)
    set_tests_properties(win32_services PROPERTIES LABELS "compat")
    add_test(NAME win32_memory COMMAND stars-compat-memory)
    set_tests_properties(win32_memory PROPERTIES LABELS "compat")
    add_test(NAME win32_dib COMMAND stars-compat-dib)
    set_tests_properties(win32_dib PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software"
        LABELS "sdl;compat" TIMEOUT 30)
    add_test(NAME sdl_smoke_software COMMAND stars-compat-smoke --self-test)
    set_tests_properties(sdl_smoke_software PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software"
        LABELS "sdl;compat"
        TIMEOUT 30
    )
endif()

add_executable(stars-compat-regions tests/compat/win32_regions.c)
target_link_libraries(stars-compat-regions PRIVATE stars_win32)
target_compile_options(stars-compat-regions PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-regions PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_regions COMMAND stars-compat-regions)
    set_tests_properties(win32_regions PROPERTIES LABELS "compat")
endif()

add_executable(stars-compat-drawing-state tests/compat/win32_drawing_state.c)
target_link_libraries(stars-compat-drawing-state PRIVATE stars_win32)
target_compile_options(stars-compat-drawing-state PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-drawing-state PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_drawing_state COMMAND stars-compat-drawing-state)
    set_tests_properties(win32_drawing_state PROPERTIES LABELS "compat")
endif()

add_executable(stars-compat-geometry tests/compat/win32_geometry.c)
target_link_libraries(stars-compat-geometry PRIVATE stars_win32)
target_compile_options(stars-compat-geometry PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-geometry PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_geometry COMMAND stars-compat-geometry)
    set_tests_properties(win32_geometry PROPERTIES LABELS "compat")
endif()

add_executable(stars-compat-children tests/compat/win32_children.c)
target_link_libraries(stars-compat-children PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-children PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-children PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-children PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_children COMMAND stars-compat-children)
    set_tests_properties(win32_children PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software")
endif()

add_executable(stars-compat-position tests/compat/win32_position.c)
target_link_libraries(stars-compat-position PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-position PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-position PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-position PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_position COMMAND stars-compat-position)
    set_tests_properties(win32_position PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software")
endif()

add_executable(stars-compat-state tests/compat/win32_state.c)
target_link_libraries(stars-compat-state PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-state PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-state PRIVATE -Wall -Wextra -Wpedantic -Werror)
stars_test_redirect(stars-compat-state SDL_GetDisplayBounds SDL_GetDisplayUsableBounds)
set_target_properties(stars-compat-state PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_state COMMAND stars-compat-state)
    set_tests_properties(win32_state PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software")
endif()

add_executable(stars-compat-capture tests/compat/win32_capture.c)
target_link_libraries(stars-compat-capture PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-capture PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-capture PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-capture PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_capture COMMAND stars-compat-capture)
    set_tests_properties(win32_capture PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software")
endif()

add_executable(stars-compat-input tests/compat/win32_input.c)
target_link_libraries(stars-compat-input PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-input PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-input PRIVATE -Wall -Wextra -Wpedantic -Werror)
stars_test_redirect(stars-compat-input SDL_GetKeyboardState SDL_GetModState SDL_GetGlobalMouseState SDL_GetCurrentVideoDriver)
set_target_properties(stars-compat-input PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_input COMMAND stars-compat-input)
    set_tests_properties(win32_input PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software")
endif()

# Poison untouched locals so the failed-query regression detects the old read.
include(CheckCCompilerFlag)
check_c_compiler_flag("-ftrivial-auto-var-init=pattern" STARS_C_PATTERN_INIT)
add_executable(stars-compat-nativeui-points tests/compat/nativeui_points.c)
target_link_libraries(stars-compat-nativeui-points PRIVATE stars_win32)
target_compile_options(stars-compat-nativeui-points PRIVATE -Wall -Wextra -Wpedantic -Werror)
if(STARS_C_PATTERN_INIT)
    target_compile_options(stars-compat-nativeui-points PRIVATE -ftrivial-auto-var-init=pattern)
endif()
set_target_properties(stars-compat-nativeui-points PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_nativeui_points COMMAND stars-compat-nativeui-points)
    set_tests_properties(win32_nativeui_points PROPERTIES LABELS "compat")
endif()

# Optional integration test; Xvfb isolates synthesized keys from the user's desktop.
find_package(PkgConfig REQUIRED)
pkg_check_modules(STARS_XTEST QUIET IMPORTED_TARGET xtst x11)
find_program(STARS_XVFB_EXECUTABLE xvfb-run)
if(STARS_XTEST_FOUND AND STARS_XVFB_EXECUTABLE)
    add_executable(stars-compat-input-x11 tests/compat/win32_input_x11.c)
    target_link_libraries(stars-compat-input-x11 PRIVATE stars_win32 SDL3::SDL3 PkgConfig::STARS_XTEST)
    target_include_directories(stars-compat-input-x11 PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
    target_compile_options(stars-compat-input-x11 PRIVATE -Wall -Wextra -Wpedantic -Werror)
    set_target_properties(stars-compat-input-x11 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_dependencies(stars-compat-input stars-compat-input-x11)
    if(STARS_BUILD_TESTS)
        add_test(NAME win32_input_x11 COMMAND "${STARS_XVFB_EXECUTABLE}" -a $<TARGET_FILE:stars-compat-input-x11>)
        set_tests_properties(win32_input_x11 PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=x11;STARS_SDL_RENDERER=software")
    endif()
endif()

add_executable(stars-compat-controls tests/compat/win32_controls.c)
target_link_libraries(stars-compat-controls PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-controls PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-controls PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-controls PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_controls COMMAND stars-compat-controls)
    set_tests_properties(win32_controls PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software")
endif()

add_executable(stars-compat-edit tests/compat/win32_edit.c)
target_link_libraries(stars-compat-edit PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-edit PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-edit PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-edit PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_edit COMMAND stars-compat-edit)
    set_tests_properties(win32_edit PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software")
endif()

add_executable(stars-compat-list tests/compat/win32_list.c)
target_link_libraries(stars-compat-list PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-list PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-list PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-list PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_list COMMAND stars-compat-list)
    set_tests_properties(win32_list PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software")
endif()

add_executable(stars-compat-desktop tests/compat/win32_desktop.c)
target_link_libraries(stars-compat-desktop PRIVATE stars_win32 SDL3::SDL3)
stars_test_redirect(stars-compat-desktop SDL_GetDisplays SDL_GetPrimaryDisplay SDL_GetDisplayBounds SDL_GetDisplayUsableBounds)
target_compile_options(stars-compat-desktop PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-desktop PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_desktop COMMAND stars-compat-desktop)
    set_tests_properties(win32_desktop PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software")
endif()

add_executable(stars-compat-attention tests/compat/win32_attention.c)
target_link_libraries(stars-compat-attention PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-attention PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-attention PRIVATE -Wall -Wextra -Wpedantic -Werror)
stars_test_redirect(stars-compat-attention SDL_FlashWindow)
set_target_properties(stars-compat-attention PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_XTEST_FOUND)
    target_link_libraries(stars-compat-attention PRIVATE PkgConfig::STARS_XTEST)
    target_compile_definitions(stars-compat-attention PRIVATE STARS_TEST_ATTENTION_X11=1)
endif()
if(STARS_BUILD_TESTS)
    add_test(NAME win32_attention COMMAND stars-compat-attention)
    set_tests_properties(win32_attention PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software")
    if(STARS_XTEST_FOUND AND STARS_XVFB_EXECUTABLE)
        add_test(NAME win32_attention_x11 COMMAND "${STARS_XVFB_EXECUTABLE}" -a $<TARGET_FILE:stars-compat-attention> --x11)
        set_tests_properties(win32_attention_x11 PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=x11;STARS_SDL_RENDERER=software" TIMEOUT 15)
    endif()
endif()

add_executable(stars-compat-scroll tests/compat/win32_scroll.c)
target_link_libraries(stars-compat-scroll PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-scroll PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-scroll PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-scroll PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_scroll COMMAND stars-compat-scroll)
    set_tests_properties(win32_scroll PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software")
endif()

add_executable(stars-compat-scrollwindow tests/compat/win32_scrollwindow.c)
target_link_libraries(stars-compat-scrollwindow PRIVATE stars_win32)
target_compile_options(stars-compat-scrollwindow PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-scrollwindow PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_scrollwindow COMMAND stars-compat-scrollwindow)
    set_tests_properties(win32_scrollwindow PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software")
endif()

add_executable(stars-compat-menu tests/compat/win32_menu.c)
target_link_libraries(stars-compat-menu PRIVATE stars_win32)
target_compile_options(stars-compat-menu PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-menu PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_menu COMMAND stars-compat-menu)
    set_tests_properties(win32_menu PROPERTIES LABELS "compat")
endif()

add_executable(stars-compat-popup tests/compat/win32_popup.c)
target_link_libraries(stars-compat-popup PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-popup PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-popup PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-popup PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_popup_x11 COMMAND stars-compat-popup)
    set_tests_properties(win32_popup_x11 PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=x11;STARS_SDL_RENDERER=software" TIMEOUT 30)
    add_test(NAME win32_popup COMMAND stars-compat-popup)
    set_tests_properties(win32_popup PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
endif()

add_executable(stars-compat-ui-resources tests/compat/win32_ui_resources.c)
target_link_libraries(stars-compat-ui-resources PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-ui-resources PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src" "${STARS_BITMAP_GENERATED}")
target_compile_options(stars-compat-ui-resources PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-ui-resources PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_ui_resources COMMAND stars-compat-ui-resources)
    set_tests_properties(win32_ui_resources PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
endif()

add_executable(stars-compat-menubar tests/compat/win32_menubar.c)
target_link_libraries(stars-compat-menubar PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-menubar PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-menubar PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-menubar PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_menubar COMMAND stars-compat-menubar)
    set_tests_properties(win32_menubar PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
    add_test(NAME win32_menubar_x11 COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_x11_wm.py"
        --xvfb "${STARS_XVFB_RUN_EXECUTABLE}" --wm "${STARS_OPENBOX_EXECUTABLE}"
        --xprop "${STARS_XPROP_EXECUTABLE}" "$<TARGET_FILE:stars-compat-menubar>")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/menubar-x11")
    set_tests_properties(win32_menubar_x11 PROPERTIES LABELS "compat"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/menubar-x11"
        ENVIRONMENT "SDL_VIDEODRIVER=x11;STARS_SDL_RENDERER=software" TIMEOUT 30)
endif()

add_executable(stars-compat-images tests/compat/win32_images.c)
target_link_libraries(stars-compat-images PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-images PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src" "${STARS_BITMAP_GENERATED}")
target_compile_options(stars-compat-images PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-images PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_images COMMAND stars-compat-images)
    set_tests_properties(win32_images PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
    add_test(NAME win32_images_x11 COMMAND stars-compat-images)
    set_tests_properties(win32_images_x11 PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=x11;STARS_SDL_RENDERER=software" TIMEOUT 30)
endif()

add_executable(stars-compat-dialog tests/compat/win32_dialog.c)
target_link_libraries(stars-compat-dialog PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-dialog PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-dialog PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_dialog COMMAND stars-compat-dialog)
    set_tests_properties(win32_dialog PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
    add_test(NAME win32_dialog_x11 COMMAND stars-compat-dialog)
    set_tests_properties(win32_dialog_x11 PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=x11;STARS_SDL_RENDERER=software" TIMEOUT 30)
endif()

add_executable(stars-compat-combo tests/compat/win32_combo.c)
target_link_libraries(stars-compat-combo PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-combo PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src" "${STARS_BITMAP_GENERATED}")
target_compile_options(stars-compat-combo PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-combo PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_combo COMMAND stars-compat-combo)
    set_tests_properties(win32_combo PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
    add_test(NAME win32_combo_x11 COMMAND stars-compat-combo)
    set_tests_properties(win32_combo_x11 PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=x11;STARS_SDL_RENDERER=software" TIMEOUT 30)
endif()

add_executable(stars-compat-palette tests/compat/win32_palette.c)
target_link_libraries(stars-compat-palette PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-palette PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-palette PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_palette COMMAND stars-compat-palette)
    set_tests_properties(win32_palette PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
    add_test(NAME win32_palette_x11 COMMAND stars-compat-palette)
    set_tests_properties(win32_palette_x11 PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=x11;STARS_SDL_RENDERER=software" TIMEOUT 30)
endif()

add_executable(stars-compat-messagebox tests/compat/win32_messagebox.c)
target_link_libraries(stars-compat-messagebox PRIVATE stars_win32 SDL3::SDL3)
stars_test_redirect(stars-compat-messagebox SDL_PutAudioStreamData)
target_include_directories(stars-compat-messagebox PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-messagebox PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-messagebox PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_messagebox COMMAND stars-compat-messagebox)
    set_tests_properties(win32_messagebox PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
    add_test(NAME win32_messagebox_x11 COMMAND stars-compat-messagebox)
    set_tests_properties(win32_messagebox_x11 PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
    add_test(NAME win32_alert_audio_failure COMMAND stars-compat-messagebox --audio-failure)
    set_tests_properties(win32_alert_audio_failure PROPERTIES LABELS "compat" ENVIRONMENT "SDL_AUDIODRIVER=stars-missing-driver" TIMEOUT 30)
endif()

if(STARS_BUILD_TESTS)
    add_test(NAME win32_alert_external_audio COMMAND stars-compat-messagebox --external-audio)
    set_tests_properties(win32_alert_external_audio PROPERTIES LABELS "compat" ENVIRONMENT "SDL_AUDIODRIVER=dummy" TIMEOUT 30)
    add_test(NAME win32_alert_muted COMMAND stars-compat-messagebox --audio-muted)
    set_tests_properties(win32_alert_muted PROPERTIES LABELS "compat" ENVIRONMENT "SDL_AUDIODRIVER=stars-missing-driver;STARS_SDL_ALERTS=off" TIMEOUT 30)
endif()

add_executable(stars-compat-filedialog tests/compat/win32_filedialog.c)
target_include_directories(stars-compat-filedialog PRIVATE compat/win32/src)
target_link_libraries(stars-compat-filedialog PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-filedialog PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-filedialog PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_filedialog COMMAND stars-compat-filedialog)
    set_tests_properties(win32_filedialog PROPERTIES LABELS "compat" ENVIRONMENT "STARS_FILE_DIALOG_BACKEND=compat;SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
    add_test(NAME win32_filedialog_x11 COMMAND stars-compat-filedialog)
    set_tests_properties(win32_filedialog_x11 PROPERTIES LABELS "compat" ENVIRONMENT "STARS_FILE_DIALOG_BACKEND=compat;SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
endif()

add_executable(stars-compat-printmap tests/compat/win32_printmap.c)
target_link_libraries(stars-compat-printmap PRIVATE stars_core stars-ui-compat-check stars_options stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-printmap PRIVATE -Wall -Wextra -Werror)
stars_test_redirect(stars-compat-printmap InitInstance PromptPassword AlertSz SDL_OpenURL)
set_target_properties(stars-compat-printmap PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_print_map COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/compat/check_print_map.py"
        "$<TARGET_FILE:stars-compat-printmap>" "${CMAKE_BINARY_DIR}/print-map-test")
    set_tests_properties(win32_print_map PROPERTIES LABELS "client;compat" TIMEOUT 150 RUN_SERIAL TRUE)
    find_program(STARS_GECKODRIVER_EXECUTABLE geckodriver)
    if(STARS_GECKODRIVER_EXECUTABLE)
        add_test(NAME win32_help_browser COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/compat/check_help_browser.py"
            "${STARS_HELP_GENERATED}/offline.html" --geckodriver "${STARS_GECKODRIVER_EXECUTABLE}")
        set_tests_properties(win32_help_browser PROPERTIES LABELS "compat" TIMEOUT 180 RUN_SERIAL TRUE)
    endif()
endif()
add_executable(stars-compat-print tests/compat/win32_print.c)
target_link_libraries(stars-compat-print PRIVATE stars_win32 SDL3::SDL3)
target_compile_definitions(stars-compat-print PRIVATE STARS_TEST_PRINT_OUTPUT="${CMAKE_BINARY_DIR}/print-test-output")
stars_test_redirect(stars-compat-print SDL_OpenURL SDL_ShowSimpleMessageBox)
target_compile_options(stars-compat-print PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-print PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_print COMMAND stars-compat-print "${CMAKE_BINARY_DIR}/print-test-output")
    set_tests_properties(win32_print PROPERTIES LABELS "compat" ENVIRONMENT "STARS_FILE_DIALOG_BACKEND=compat;SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" FIXTURES_SETUP print_document TIMEOUT 60)
endif()
if(STARS_BUILD_TESTS)
    find_program(STARS_PDFINFO_EXECUTABLE pdfinfo REQUIRED)
    find_program(STARS_PDFTOPPM_EXECUTABLE pdftoppm REQUIRED)
    add_test(NAME win32_print_pdf COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/compat/check_print_pdf.py"
        "${CMAKE_BINARY_DIR}/print-test-output" "${STARS_PDFINFO_EXECUTABLE}" "${STARS_PDFTOPPM_EXECUTABLE}")
    set_tests_properties(win32_print_pdf PROPERTIES LABELS "compat" DEPENDS win32_print FIXTURES_REQUIRED print_document TIMEOUT 30)
endif()

add_executable(stars-compat-inspect tests/compat/win32_inspect.c)
target_link_libraries(stars-compat-inspect PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-inspect PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-inspect PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-inspect PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_inspect_x11 COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_x11_wm.py"
        --xvfb "${STARS_XVFB_RUN_EXECUTABLE}" --wm "${STARS_OPENBOX_EXECUTABLE}"
        --xprop "${STARS_XPROP_EXECUTABLE}" "$<TARGET_FILE:stars-compat-inspect>")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/inspect-x11")
    set_tests_properties(win32_inspect_x11 PROPERTIES LABELS "compat"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test-client/inspect-x11"
        ENVIRONMENT "SDL_VIDEODRIVER=x11;STARS_SDL_RENDERER=software" TIMEOUT 30)
    add_test(NAME win32_inspect COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/compat/check_inspect.py"
        "$<TARGET_FILE:stars-compat-inspect>")
    set_tests_properties(win32_inspect PROPERTIES LABELS "compat"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
endif()

add_executable(stars-compat-toolbar tests/compat/win32_toolbar.c)
target_link_libraries(stars-compat-toolbar PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-toolbar PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-toolbar PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-toolbar PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_toolbar COMMAND stars-compat-toolbar)
    set_tests_properties(win32_toolbar PROPERTIES LABELS "compat" ENVIRONMENT "SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
    add_test(NAME win32_toolbar_x11 COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/compat/run_x11_wm.py"
        --xvfb "${STARS_XVFB_RUN_EXECUTABLE}" --wm "${STARS_OPENBOX_EXECUTABLE}" --xprop "${STARS_XPROP_EXECUTABLE}" "$<TARGET_FILE:stars-compat-toolbar>")
    set_tests_properties(win32_toolbar_x11 PROPERTIES LABELS "compat" ENVIRONMENT "SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software" TIMEOUT 30)
endif()

add_executable(stars-compat-fileportal tests/compat/win32_fileportal.c)
target_include_directories(stars-compat-fileportal PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_link_libraries(stars-compat-fileportal PRIVATE stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-fileportal PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-fileportal PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_NATIVE_FILE_DIALOGS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND STARS_BUILD_TESTS)
    find_program(STARS_DBUS_RUN_SESSION dbus-run-session REQUIRED)
    add_test(NAME win32_fileportal COMMAND "${STARS_DBUS_RUN_SESSION}" -- "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_fileportal.py" --client "$<TARGET_FILE:stars-compat-fileportal>")
    set_tests_properties(win32_fileportal PROPERTIES LABELS "compat" ENVIRONMENT "STARS_TEST_PRIVATE_BUS=1" TIMEOUT 35)
    add_test(NAME win32_fileworkflow COMMAND "${STARS_DBUS_RUN_SESSION}" -- "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_fileworkflow.py" --client "$<TARGET_FILE:stars-compat-fileworkflow>")
    set_tests_properties(win32_fileworkflow PROPERTIES LABELS "client" ENVIRONMENT "STARS_TEST_PRIVATE_BUS=1" TIMEOUT 55)
    if(STARS_XVFB_RUN_EXECUTABLE AND STARS_OPENBOX_EXECUTABLE AND STARS_XPROP_EXECUTABLE)
        add_test(NAME win32_fileportal_x11 COMMAND "${Python3_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/tests/compat/run_x11_wm.py"
            --xvfb "${STARS_XVFB_RUN_EXECUTABLE}" --wm "${STARS_OPENBOX_EXECUTABLE}"
            --xprop "${STARS_XPROP_EXECUTABLE}" --timeout 35 "${STARS_DBUS_RUN_SESSION}" -- "${Python3_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/tests/compat/run_fileportal.py" --client "$<TARGET_FILE:stars-compat-fileportal>" --driver x11)
        set_tests_properties(win32_fileportal_x11 PROPERTIES LABELS "compat" ENVIRONMENT "STARS_TEST_PRIVATE_BUS=1" TIMEOUT 45)
    endif()
endif()

# Weston runs privately with no visible desktop or user session dependency.
find_program(STARS_WESTON_EXECUTABLE NAMES weston)
set(STARS_TEST_MESA_PREFIX "" CACHE PATH "Private patched Mesa prefix for Wayland qualification only")
if(STARS_WESTON_EXECUTABLE AND STARS_BUILD_TESTS)
    add_executable(stars-compat-wayland tests/compat/win32_wayland.c)
    target_link_libraries(stars-compat-wayland PRIVATE stars_win32 SDL3::SDL3)
    target_include_directories(stars-compat-wayland PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
    target_compile_options(stars-compat-wayland PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_dependencies(stars-compat-windows stars-compat-wayland)
    set_target_properties(stars-compat-wayland PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    foreach(STARS_WAYLAND_SCALE 1 2)
        foreach(STARS_WAYLAND_RENDERER software opengl)
            add_test(NAME win32_desktop_wayland_${STARS_WAYLAND_SCALE}_${STARS_WAYLAND_RENDERER}
                COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/compat/run_wayland.py"
                    --weston "${STARS_WESTON_EXECUTABLE}" --scale ${STARS_WAYLAND_SCALE}
                    --renderer ${STARS_WAYLAND_RENDERER} "$<TARGET_FILE:stars-compat-wayland>")
            set_tests_properties(win32_desktop_wayland_${STARS_WAYLAND_SCALE}_${STARS_WAYLAND_RENDERER}
                PROPERTIES LABELS "compat;wayland" RUN_SERIAL TRUE TIMEOUT 60)
        endforeach()
    endforeach()
    foreach(STARS_WAYLAND_CLIENT startup race)
        add_test(NAME win32_client_${STARS_WAYLAND_CLIENT}_wayland
            COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/compat/run_wayland.py"
                --weston "${STARS_WESTON_EXECUTABLE}" "$<TARGET_FILE:stars-compat-client-startup>")
        set_tests_properties(win32_client_${STARS_WAYLAND_CLIENT}_wayland PROPERTIES
            LABELS "client;wayland" RUN_SERIAL TRUE TIMEOUT 60)
    endforeach()
    set_tests_properties(win32_client_race_wayland PROPERTIES
        ENVIRONMENT "STARS_CLIENT_RACE_CAPTURE=${CMAKE_BINARY_DIR}/test-client/habitat-wayland.bmp")
    add_test(NAME sdl_smoke_wayland
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/compat/run_wayland.py"
            --weston "${STARS_WESTON_EXECUTABLE}" "$<TARGET_FILE:stars-compat-smoke>" --self-test)
    set_tests_properties(sdl_smoke_wayland PROPERTIES LABELS "compat;wayland" RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME win32_client_designer_wayland
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/compat/run_wayland.py"
            --weston "${STARS_WESTON_EXECUTABLE}" --timeout 200 "${Python3_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/tests/compat/run_client_continue.py" "$<TARGET_FILE:stars-compat-client-startup>")
    set_tests_properties(win32_client_designer_wayland PROPERTIES LABELS "client;wayland" RUN_SERIAL TRUE TIMEOUT 210
        ENVIRONMENT "STARS_CLIENT_WORKFLOW=designer")
    if(STARS_TEST_MESA_PREFIX)
        set_property(TEST sdl_smoke_wayland win32_client_startup_wayland win32_client_race_wayland win32_client_designer_wayland
            win32_desktop_wayland_1_software win32_desktop_wayland_1_opengl
            win32_desktop_wayland_2_software win32_desktop_wayland_2_opengl
            APPEND PROPERTY ENVIRONMENT "STARS_TEST_MESA_PREFIX=${STARS_TEST_MESA_PREFIX}")
    endif()
endif()

add_executable(stars-compat-utf8 tests/compat/win32_utf8.c)
target_link_libraries(stars-compat-utf8 PRIVATE stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-utf8 PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_options(stars-compat-utf8 PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(stars-compat-utf8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_dependencies(stars-compat-filedialog stars-compat-utf8)
if(STARS_BUILD_TESTS)
    add_test(NAME win32_utf8 COMMAND stars-compat-utf8)
    set_tests_properties(win32_utf8 PROPERTIES LABELS "compat" TIMEOUT 30
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat")
endif()
