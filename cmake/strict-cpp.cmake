# Every owned production source, including generated tables, receives the
# same strict diagnostics. Source properties
# avoid exporting production warning policy to independent C/SDK consumers.
function(stars_apply_strict_cpp_directory directory)
    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        get_target_property(sources "${target}" SOURCES)
        get_target_property(source_directory "${target}" SOURCE_DIR)
        foreach(source IN LISTS sources)
            if(source MATCHES "^\\$<")
                continue()
            endif()
            get_filename_component(absolute "${source}" ABSOLUTE BASE_DIR "${source_directory}")
            file(RELATIVE_PATH source_name "${PROJECT_SOURCE_DIR}" "${absolute}")
            file(RELATIVE_PATH generated_name "${CMAKE_BINARY_DIR}" "${absolute}")
            if(source_name MATCHES "^(src/|compat/win32/src/)" OR
               generated_name MATCHES "^(generated/|compat/win32/generated/)")
                get_source_file_property(applied "${absolute}" TARGET_DIRECTORY "${target}" STARS_STRICT_CPP)
                if(NOT applied)
                    set_property(SOURCE "${absolute}" TARGET_DIRECTORY "${target}" APPEND PROPERTY COMPILE_OPTIONS
                        "$<$<COMPILE_LANGUAGE:CXX,OBJCXX>:-Wpedantic>"
                        "$<$<COMPILE_LANGUAGE:CXX,OBJCXX>:-Wold-style-cast>"
                        "$<$<COMPILE_LANGUAGE:CXX,OBJCXX>:-Wzero-as-null-pointer-constant>"
                        "$<$<COMPILE_LANGUAGE:CXX,OBJCXX>:-Werror>")
                    set_source_files_properties("${absolute}" TARGET_DIRECTORY "${target}" PROPERTIES STARS_STRICT_CPP TRUE)
                endif()
            endif()
        endforeach()
    endforeach()
    get_property(children DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(child IN LISTS children)
        stars_apply_strict_cpp_directory("${child}")
    endforeach()
endfunction()
stars_apply_strict_cpp_directory("${PROJECT_SOURCE_DIR}")
