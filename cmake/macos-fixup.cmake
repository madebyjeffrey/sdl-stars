cmake_minimum_required(VERSION 3.23)
# Script mode is required: dependency copying must not run during configure.
# Copy framework runtime code/resources and preserve its versioned symlinks.
set(BU_COPY_FULL_FRAMEWORK_CONTENTS OFF)
include(BundleUtilities)
fixup_bundle("${STARS_APP}" "" "${STARS_LIBRARY_DIRS}")
