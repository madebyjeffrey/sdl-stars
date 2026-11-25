# The PDF map workflow runs without a native print dialog.
add_executable(stars-compat-printmap tests/compat/win32_printmap.cpp)
target_link_libraries(stars-compat-printmap PRIVATE stars_core stars-ui-compat-check stars_options stars_win32 SDL3::SDL3)
target_compile_options(stars-compat-printmap PRIVATE -Wall -Wextra -Werror)

set_target_properties(stars-compat-printmap PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_print_map COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/compat/check_print_map.py"
        "$<TARGET_FILE:stars-compat-printmap>" "${CMAKE_BINARY_DIR}/print-map-test")
    set_tests_properties(win32_print_map PROPERTIES LABELS "client;compat" TIMEOUT 150 RUN_SERIAL TRUE)
endif()
