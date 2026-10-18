set(STARS_HELP_GENERATED "${CMAKE_BINARY_DIR}/help")
add_custom_command(OUTPUT "${STARS_HELP_GENERATED}/complete.stamp" "${STARS_HELP_GENERATED}/help_contexts.h"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/../compat/win32/helpgen.py"
        "${CMAKE_CURRENT_LIST_DIR}/../res/help-utf8.zip" "${STARS_HELP_GENERATED}"
    DEPENDS "${CMAKE_CURRENT_LIST_DIR}/../res/help-utf8.zip" "${CMAKE_CURRENT_LIST_DIR}/../compat/win32/helpgen.py"
    COMMENT "Staging the UTF-8 Stars! help guide" VERBATIM)
# Ninja's built-in help target collides with this generated directory. Remove
# the complete asset tree before its ordinary file cleanup, including old assets.
set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${STARS_HELP_GENERATED}")
add_custom_target(stars-help-guide ALL DEPENDS "${STARS_HELP_GENERATED}/complete.stamp")
