# Linux already registers these portable contracts in sdl-client.cmake. Keep the
# same image/font/printing cases on Apple and Windows during ownership changes.
foreach(STARS_RESOURCE_KIND images font print)
    set(STARS_RESOURCE_TARGET stars-resource-${STARS_RESOURCE_KIND})
    add_executable(${STARS_RESOURCE_TARGET} tests/compat/win32_${STARS_RESOURCE_KIND}.c)
    target_link_libraries(${STARS_RESOURCE_TARGET} PRIVATE stars_win32 SDL3::SDL3)
    target_include_directories(${STARS_RESOURCE_TARGET} PRIVATE
        "${PROJECT_SOURCE_DIR}/compat/win32/src" "${STARS_BITMAP_GENERATED}")
    target_compile_options(${STARS_RESOURCE_TARGET} PRIVATE -Wall -Wextra -Werror)
    set_target_properties(${STARS_RESOURCE_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_test(NAME win32_sdl_resource_${STARS_RESOURCE_KIND} COMMAND ${STARS_RESOURCE_TARGET})
    set_tests_properties(win32_sdl_resource_${STARS_RESOURCE_KIND} PROPERTIES LABELS "compat;ownership"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat" TIMEOUT 60)
endforeach()
set(STARS_RESOURCE_PRINT_DIR "${CMAKE_BINARY_DIR}/resource-print-output")
target_compile_definitions(stars-resource-print PRIVATE STARS_TEST_PRINT_OUTPUT="${STARS_RESOURCE_PRINT_DIR}")
stars_test_redirect(stars-resource-print SDL_OpenURL SDL_ShowSimpleMessageBox)
set_tests_properties(win32_sdl_resource_print PROPERTIES FIXTURES_SETUP resource_print_document)
find_program(STARS_RESOURCE_PDFINFO pdfinfo)
find_program(STARS_RESOURCE_PDFTOPPM pdftoppm)
if(STARS_RESOURCE_PDFINFO AND STARS_RESOURCE_PDFTOPPM)
    add_test(NAME win32_sdl_resource_print_pdf COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/check_print_pdf.py" "${STARS_RESOURCE_PRINT_DIR}"
        "${STARS_RESOURCE_PDFINFO}" "${STARS_RESOURCE_PDFTOPPM}")
    set_tests_properties(win32_sdl_resource_print_pdf PROPERTIES LABELS "compat;ownership"
        FIXTURES_REQUIRED resource_print_document TIMEOUT 30)
endif()
if(APPLE AND NOT STARS_ENABLE_SANITIZERS)
    # leaks inspects the native allocator; ASan uses a separate allocator/check.
    find_program(STARS_LEAKS_EXECUTABLE leaks REQUIRED)
    foreach(STARS_LEAK_KIND images font print faults)
        set(STARS_LEAK_ARGS)
        if(STARS_LEAK_KIND STREQUAL "print")
            list(APPEND STARS_LEAK_ARGS "${CMAKE_BINARY_DIR}/resource-leaks-print-output")
        endif()
        add_test(NAME win32_sdl_leaks_${STARS_LEAK_KIND} COMMAND "${STARS_LEAKS_EXECUTABLE}"
            --quiet --noContent --atExit -- "$<TARGET_FILE:stars-resource-${STARS_LEAK_KIND}>" ${STARS_LEAK_ARGS})
        set_tests_properties(win32_sdl_leaks_${STARS_LEAK_KIND} PROPERTIES LABELS "compat;ownership;leaks"
            ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_FILE_DIALOG_BACKEND=compat"
            RUN_SERIAL TRUE TIMEOUT 60)
    endforeach()
endif()
