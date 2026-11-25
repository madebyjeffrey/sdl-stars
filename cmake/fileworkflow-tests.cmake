# The portable chooser replays the same real file workflow as the Linux portal.
add_executable(stars-compat-fileworkflow tests/compat/win32_fileworkflow.cpp)
target_link_libraries(stars-compat-fileworkflow PRIVATE stars_core stars-ui-compat-check stars_options stars_win32 SDL3::SDL3)
target_include_directories(stars-compat-fileworkflow PRIVATE "${PROJECT_SOURCE_DIR}/compat/win32/src")
target_compile_definitions(stars-compat-fileworkflow PRIVATE STARS_TEST_FILE_WORKFLOW=1)
target_compile_options(stars-compat-fileworkflow PRIVATE -Wall -Wextra -Werror)

set_target_properties(stars-compat-fileworkflow PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(STARS_BUILD_TESTS)
    add_test(NAME win32_client_fileworkflow_dummy COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tests/compat/run_fileworkflow_compat.py" --client "$<TARGET_FILE:stars-compat-fileworkflow>")
    set_tests_properties(win32_client_fileworkflow_dummy PROPERTIES LABELS client TIMEOUT 90)
endif()
