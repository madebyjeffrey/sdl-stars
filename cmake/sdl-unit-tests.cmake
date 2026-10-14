if(STARS_BUILD_TESTS)
    set(STARS_CLIENT_TEST_DIR "${CMAKE_BINARY_DIR}/tests-client")
    file(MAKE_DIRECTORY "${STARS_CLIENT_TEST_DIR}/data")
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/scaffold/utf8_fixtures.py"
        "${PROJECT_SOURCE_DIR}/tests/scaffold/fixtures/newgame/tiny/humanoid.r1"
        "${STARS_CLIENT_TEST_DIR}/data/humanoid.r1" COMMAND_ERROR_IS_FATAL ANY)
    file(GLOB STARS_CLIENT_UNIT_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/tests/unit/test_*.c")
    set(STARS_CLIENT_TEST_TARGETS)
    foreach(STARS_CLIENT_UNIT_SOURCE ${STARS_CLIENT_UNIT_SOURCES})
        get_filename_component(STARS_CLIENT_TEST_NAME "${STARS_CLIENT_UNIT_SOURCE}" NAME_WE)
        set(STARS_CLIENT_TEST_TARGET "sdl_${STARS_CLIENT_TEST_NAME}")
        add_executable(${STARS_CLIENT_TEST_TARGET} "${STARS_CLIENT_UNIT_SOURCE}" tests/unit/stars_test.c)
        target_link_libraries(${STARS_CLIENT_TEST_TARGET} PRIVATE stars_core stars-ui-compat-check
            stars_options stars_win32 SDL3::SDL3)
        target_include_directories(${STARS_CLIENT_TEST_TARGET} PRIVATE "${PROJECT_SOURCE_DIR}/tests/unit")
        target_compile_definitions(${STARS_CLIENT_TEST_TARGET} PRIVATE
            STARS_TEST_GOLDEN_DIR="${PROJECT_SOURCE_DIR}/tests/unit/golden")
        stars_test_redirect(${STARS_CLIENT_TEST_TARGET} AlertSz)
        set_target_properties(${STARS_CLIENT_TEST_TARGET} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${STARS_CLIENT_TEST_DIR}")
        list(APPEND STARS_CLIENT_TEST_TARGETS ${STARS_CLIENT_TEST_TARGET})
        if(WIN32)
            # Acutest's SDK process/timer APIs cannot consume wrapper handles.
            target_compile_definitions(${STARS_CLIENT_TEST_TARGET} PRIVATE STARS_USE_WIN_STUBS=1)
            add_test(NAME ${STARS_CLIENT_TEST_TARGET} COMMAND "${Python3_EXECUTABLE}"
                "${PROJECT_SOURCE_DIR}/tests/unit/run_isolated.py" "$<TARGET_FILE:${STARS_CLIENT_TEST_TARGET}>")
        else()
            add_test(NAME ${STARS_CLIENT_TEST_TARGET} COMMAND ${STARS_CLIENT_TEST_TARGET})
        endif()
        set_tests_properties(${STARS_CLIENT_TEST_TARGET} PROPERTIES LABELS "client"
            WORKING_DIRECTORY "${STARS_CLIENT_TEST_DIR}"
            ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat"
            TIMEOUT 60)
    endforeach()
    stars_test_redirect(sdl_test_native_ports WriteRt ReadRt FCreateFile StreamOpen StreamClose DirtyGame)
    add_custom_target(stars-sdl-unit-tests DEPENDS ${STARS_CLIENT_TEST_TARGETS})
    add_executable(stars_scenario tests/scenarios/scenarios.c tests/unit/stars_test.c)
    target_link_libraries(stars_scenario PRIVATE stars_core stars-ui-compat-check stars_options stars_win32 SDL3::SDL3)
    target_include_directories(stars_scenario PRIVATE "${PROJECT_SOURCE_DIR}/tests/unit")
    stars_test_redirect(stars_scenario AlertSz)
    set_target_properties(stars_scenario PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_CLIENT_TEST_DIR}")
endif()
