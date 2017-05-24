#.rst:
# FindBreakpad
# -------
# Finds the Google Breakpad library
#
# This will will define the following variables::
#
# BREAKPAD_FOUND - system has BREAKPAD
# BREAKPAD_INCLUDE_DIRS - the BREAKPAD include directory
# BREAKPAD_LIBRARIES    - the BREAKPAD library
# BREAKPAD_DEFINITIONS - the BREAKPAD definitions
#
# and the following imported targets::
#
#   BREAKPAD::BREAKPAD   - The BREAKPAD codec

if(CMAKE_BUILD_TYPE STREQUAL Release)

  find_path(BREAKPAD_INCLUDE_DIR client/linux/handler/minidump_descriptor.h
                          PATH_SUFFIXES breakpad)
  find_library(BREAKPAD_LIBRARY NAMES breakpad_client)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(BREAKPAD
                                  REQUIRED_VARS BREAKPAD_INCLUDE_DIR BREAKPAD_LIBRARY)

  if(BREAKPAD_FOUND)
    set(BREAKPAD_INCLUDE_DIRS ${BREAKPAD_INCLUDE_DIR})
    set(BREAKPAD_LIBRARIES ${BREAKPAD_LIBRARY})
    set(BREAKPAD_DEFINITIONS -DHAVE_BREAKPAD=1)

    if(NOT TARGET BREAKPAD::BREAKPAD)
      include_directories(${BREAKPAD_INCLUDE_DIR})
      add_library(BREAKPAD::BREAKPAD UNKNOWN IMPORTED)
      set_target_properties(BREAKPAD::BREAKPAD PROPERTIES
                                   INTERFACE_INCLUDE_DIRECTORIES "${BREAKPAD_INCLUDE_DIR}"
                                   INTERFACE_LIBRARIES "${BREAKPAD_LIBRARY}"
                                   INTERFACE_COMPILE_DEFINITIONS HAVE_BREAKPAD=1)
    endif()
  endif()
endif()

mark_as_advanced(BREAKPAD_INCLUDE_DIR BREAKPAD_LIBRARY)
